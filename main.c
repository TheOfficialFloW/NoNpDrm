/*
  NoNpDrm Plugin
  Copyright (C) 2017-2018, TheFloW

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/sysclib.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/io/dirent.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/io/stat.h>
#include <taihen.h>

#define FAKE_AID 0x0123456789ABCDEFLL

static tai_hook_ref_t ksceKernelLaunchAppRef;
static tai_hook_ref_t ksceNpDrmGetRifInfoRef;
static tai_hook_ref_t ksceNpDrmGetRifVitaKeyRef;
static tai_hook_ref_t ksceNpDrmGetRifNameRef;
static tai_hook_ref_t ksceNpDrmGetRifNameForInstallRef;

static SceUID hooks[5];
static int n_hooks = 0;

typedef struct {
  uint16_t version;                 // 0x00
  uint16_t version_flag;            // 0x02
  uint16_t type;                    // 0x04
  uint16_t flags;                   // 0x06
  uint64_t aid;                     // 0x08
  char content_id[0x30];            // 0x10
  uint8_t key_table[0x10];          // 0x40
  uint8_t key[0x10];                // 0x50
  uint64_t start_time;              // 0x60
  uint64_t expiration_time;         // 0x68
  uint8_t ecdsa_signature[0x28];    // 0x70

  uint64_t flags2;                  // 0x98
  uint8_t key2[0x10];               // 0xA0
  uint8_t unk_B0[0x10];             // 0xB0
  uint8_t openpsid[0x10];           // 0xC0
  uint8_t unk_D0[0x10];             // 0xD0
  uint8_t cmd56_handshake[0x14];    // 0xE0
  uint32_t unk_F4;                  // 0xF4
  uint32_t unk_F8;                  // 0xF8
  uint32_t sku_flag;                // 0xFC
  uint8_t rsa_signature[0x100];     // 0x100
} SceNpDrmLicense;

int ksceNpDrmGetFixedRifName(char *rif_name, SceUInt64 aid);
int ksceNpDrmGetRifVitaKey(SceNpDrmLicense *license_buf, uint8_t *klicensee, uint32_t *flags,
                           uint32_t *sku_flag, uint64_t *start_time, uint64_t *expiration_time);

static int MakeFakeLicense(char *license_path, SceNpDrmLicense *license_buf) {
  int res;
  char path[512];
  char rif_name[48];
  uint8_t klicensee[0x10];

  // Get klicensee
  memset(klicensee, 0, sizeof(klicensee));
  res = ksceNpDrmGetRifVitaKey(license_buf, klicensee, NULL, NULL, NULL, NULL);
  if (res < 0)
    return res;

  // Check validity of klicensee
  int count = 0;

  int i;
  for (i = 0; i < sizeof(klicensee); i++) {
    if (klicensee[i] == 0)
      count++;
  }

  if (count == sizeof(klicensee))
    return -1;

  // Get fixed rif name
  res = ksceNpDrmGetFixedRifName(rif_name, 0LL);
  if (res < 0)
    return res;

  // Make path
  char *p = strchr(license_path, ':');
  if (!p)
    return -2;

  snprintf(path, sizeof(path), "ux0:nonpdrm/%s/%s", p + 1, rif_name);

  SceIoStat stat;
  if(ksceIoGetstat(path, &stat) >= 0)
    return 0; // Already has fake license

  // Make license structure
  SceNpDrmLicense license;
  memset(&license, 0, sizeof(SceNpDrmLicense));
  license.aid           = FAKE_AID;
  license.version       = __builtin_bswap16(1);
  license.version_flag  = __builtin_bswap16(1);
  license.type          = __builtin_bswap16(1);
  license.flags         = __builtin_bswap16(__builtin_bswap16(license_buf->flags) & ~0x400);
  license.flags2        = license_buf->flags2;

  if (__builtin_bswap32(license_buf->sku_flag) == 1 ||
      __builtin_bswap32(license_buf->sku_flag) == 3)
    license.sku_flag = __builtin_bswap32(3);
  else
    license.sku_flag = __builtin_bswap32(0);

  memcpy(license.content_id, license_buf->content_id, 0x30);
  memcpy(license.key, klicensee, 0x10);

  // Write fake license
  char *c;
  for (c = path; *c; c++) {
    if (*c == '/') {
      *c = '\0';
      ksceIoMkdir(path, 0777);
      *c = '/';
    }
  }

  SceUID fd = ksceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
  if (fd < 0)
    return fd;

  ksceIoWrite(fd, &license, sizeof(SceNpDrmLicense));
  ksceIoClose(fd);

  return 0;
}

static int FindLicenses(char *path) {
  SceUID dfd = ksceIoDopen(path);
  if (dfd < 0)
    return dfd;

  int res = 0;

  do {
    SceIoDirent dir;
    memset(&dir, 0, sizeof(SceIoDirent));

    res = ksceIoDread(dfd, &dir);
    if (res > 0) {
      char new_path[512];
      snprintf(new_path, sizeof(new_path), "%s/%s", path, dir.d_name);

      if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
        FindLicenses(new_path);
      } else {
        SceUID fd = ksceIoOpen(new_path, SCE_O_RDONLY, 0);
        if (fd >= 0) {
          SceNpDrmLicense license;
          int size = ksceIoRead(fd, &license, sizeof(SceNpDrmLicense));
          ksceIoClose(fd);

          if (size == sizeof(SceNpDrmLicense) && license.aid != FAKE_AID)
            MakeFakeLicense(path, &license);
        }
      }
    }
  } while (res > 0);

  ksceIoDclose(dfd);

  return 0;
}

static int SetAddcontId (char* const addcontid, char const* const titleid) {
  /* Copyright (C) 2019 Fumu-no-Kagomeko */
  struct {
    uint32_t magic;
    uint32_t version;
    uint32_t keys_off;
    uint32_t data_off;
    uint32_t entries;
  } sfo_hdr;
  struct {
    uint16_t keys_off;
    uint16_t data_fmt;
    uint32_t data_len;
    uint32_t data_max;
    uint32_t data_off;
  } entry;
  int size;
  SceUID fd;
  char data[48];

  snprintf(data, sizeof(data), "ux0:app/%s/sce_sys/param.sfo", titleid);
  fd = ksceIoOpen(data, SCE_O_RDONLY, 0);

  if (fd < 0)
  {
    snprintf(data, sizeof(data), "gro0:app/%s/sce_sys/param.sfo", titleid);
    fd = ksceIoOpen(data, SCE_O_RDONLY, 0);

    if (fd < 0)
    {
      return 0;
    }
  }

  size = ksceIoRead(fd, &sfo_hdr, sizeof(sfo_hdr));

  if (size != sizeof(sfo_hdr) || sfo_hdr.magic != 0x46535000)
  {
    goto not_found;
  }

  for (uint32_t i = 0; i < sfo_hdr.entries; ++i)
  {
    size = 0x14 + sizeof(entry) * i;
    size = ksceIoPread(fd, &entry, sizeof(entry), size);

    if (size < 0 || size != sizeof(entry))
    {
      goto not_found;
    }
    else
    if (entry.data_max != 12 || entry.data_fmt != 0x0204)
    {
      continue;
    }

    size = sfo_hdr.keys_off + entry.keys_off;
    size = ksceIoPread(fd, data, 32, size);

    if (size < 0)
    {
      goto not_found;
    }
    else
    if (size < 20 || strncmp(data, "INSTALL_DIR_ADDCONT", 32))
    {
      continue;
    }

    size = sfo_hdr.data_off + entry.data_off;
    size = ksceIoPread(fd, data, 12, size);

    if (size != 12)
    {
      goto not_found;
    }

    memcpy(addcontid, data, 12);
    addcontid[11] = 0;
    ksceIoClose(fd);
    return 1;
  }

not_found:
  ksceIoClose(fd);
  return 0;
}

static SceUID _ksceKernelLaunchAppPatched(void *args) {
  char *titleid  = (char *)((uintptr_t *)args)[0];
  uint32_t flags = (uint32_t)((uintptr_t *)args)[1];
  char *path     = (char *)((uintptr_t *)args)[2];
  void *unk      = (void *)((uintptr_t *)args)[3];

  char license_path[512];
  char addcontid[12];

  snprintf(license_path, sizeof(license_path), "ux0:license/app/%s", titleid);
  FindLicenses(license_path);

  snprintf(license_path, sizeof(license_path), "gro0:license/app/%s", titleid);
  FindLicenses(license_path);

  snprintf(license_path, sizeof(license_path), "ux0:license/addcont/%s", titleid);
  FindLicenses(license_path);

  snprintf(license_path, sizeof(license_path), "grw0:license/addcont/%s", titleid);
  FindLicenses(license_path);

  if (SetAddcontId(addcontid, titleid))
  {
    snprintf(license_path, sizeof(license_path), "ux0:license/addcont/%s", addcontid);
    FindLicenses(license_path);

    snprintf(license_path, sizeof(license_path), "grw0:license/addcont/%s", addcontid);
    FindLicenses(license_path);
  }

  return TAI_CONTINUE(int, ksceKernelLaunchAppRef, titleid, flags, path, unk); // returns pid
}

static SceUID ksceKernelLaunchAppPatched(char *titleid, uint32_t flags, char *path, void *unk) {
  uintptr_t args[4];
  args[0] = (uintptr_t)titleid;
  args[1] = (uintptr_t)flags;
  args[2] = (uintptr_t)path;
  args[3] = (uintptr_t)unk;

  return ksceKernelRunWithStack(0x4000, _ksceKernelLaunchAppPatched, args);
}

static int ksceNpDrmGetRifInfoPatched(SceNpDrmLicense *license_buf, int license_size,
                                      int mode, char *content_id, uint64_t *aid,
                                      uint16_t *license_version, uint8_t *license_flags,
                                      uint32_t *flags, uint32_t *sku_flag,
                                      uint64_t *start_time, uint64_t *expiration_time,
                                      uint64_t *flags2) {
  int res = TAI_CONTINUE(int, ksceNpDrmGetRifInfoRef, license_buf, license_size,
                                                      mode, content_id, aid,
                                                      license_version, license_flags,
                                                      flags, sku_flag,
                                                      start_time, expiration_time,
                                                      flags2);

  // Trial version -> Full version
  if (sku_flag) {
    if (__builtin_bswap32(license_buf->sku_flag) == 1 ||
        __builtin_bswap32(license_buf->sku_flag) == 3)
      *sku_flag = 3;
    else
      *sku_flag = 0;
  }

  // Bypass expiration time for PS Plus games
  if (start_time)
    *start_time = 0LL;
  if (expiration_time)
    *expiration_time = 0x7FFFFFFFFFFFFFFFLL;

  // Get fake rif info and return success
  if (res < 0 && license_buf && license_buf->aid == FAKE_AID) {
    if (content_id)
      memcpy(content_id, license_buf->content_id, 0x30);

    if (flags) {
      if (__builtin_bswap16(license_buf->flags) & 0x200)
        (*flags) |= 0x1;
      if (__builtin_bswap16(license_buf->flags) & 0x100)
        (*flags) |= 0x10000;
      if (__builtin_bswap64(license_buf->flags2) & 0x1)
        (*flags) |= 0x2;
    }

    if (flags2)
      *flags2 = __builtin_bswap64(license_buf->flags2) & ~0x1;

    if (license_version)
      *license_version = __builtin_bswap16(license_buf->version);
    if (license_flags)
      *license_flags = (uint8_t)__builtin_bswap16(license_buf->flags);

    if (aid)
      *aid = 0LL;

    return 0;
  }

  return res;
}

static int ksceNpDrmGetRifVitaKeyPatched(SceNpDrmLicense *license_buf, uint8_t *klicensee,
                                         uint32_t *flags, uint32_t *sku_flag,
                                         uint64_t *start_time, uint64_t *expiration_time) {
  int res = TAI_CONTINUE(int, ksceNpDrmGetRifVitaKeyRef, license_buf, klicensee,
                                                         flags, sku_flag,
                                                         start_time, expiration_time);

  // Trial version -> Full version
  if (sku_flag) {
    if (__builtin_bswap32(license_buf->sku_flag) == 1 ||
        __builtin_bswap32(license_buf->sku_flag) == 3)
      *sku_flag = 3;
    else
      *sku_flag = 0;
  }

  // Bypass expiration time for PS Plus games
  if (start_time)
    *start_time = 0LL;
  if (expiration_time)
    *expiration_time = 0x7FFFFFFFFFFFFFFFLL;

  // Get fake rif info and klicensee and return success
  if (res < 0 && license_buf && license_buf->aid == FAKE_AID) {
    if (klicensee)
      memcpy(klicensee, license_buf->key, 0x10);

    if (flags) {
      if (__builtin_bswap16(license_buf->flags) & 0x200)
        (*flags) |= 0x1;
      if (__builtin_bswap16(license_buf->flags) & 0x100)
        (*flags) |= 0x10000;
      if (__builtin_bswap64(license_buf->flags2) & 0x1)
        (*flags) |= 0x2;
    }

    return 0;
  }

  return res;
}

static int ksceNpDrmGetRifNamePatched(char *rif_name, uint32_t flags, uint64_t aid) {
  int res = TAI_CONTINUE(int, ksceNpDrmGetRifNameRef, rif_name, flags, aid);

  // Allow applications on non-activated devices by using fixed rif name
  if (res < 0)
    return ksceNpDrmGetFixedRifName(rif_name, 0LL);

  return res;
}

static int ksceNpDrmGetRifNameForInstallPatched(char *rif_name, SceNpDrmLicense *license_buf, uint32_t flags) {
  int res = TAI_CONTINUE(int, ksceNpDrmGetRifNameForInstallRef, rif_name, license_buf, flags);

  // Use fixed rif name for fake license
  if (license_buf && license_buf->aid == FAKE_AID)
    return ksceNpDrmGetFixedRifName(rif_name, 0LL);

  return res;
}

void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
  hooks[n_hooks] = taiHookFunctionExportForKernel(KERNEL_PID, &ksceKernelLaunchAppRef, "SceProcessmgr",
                                                  0x7A69DE86, 0x71CF71FD, ksceKernelLaunchAppPatched);
  if (hooks[n_hooks] < 0)
    hooks[n_hooks] = taiHookFunctionExportForKernel(KERNEL_PID, &ksceKernelLaunchAppRef, "SceProcessmgr",
                                                    0xEB1F8EF7, 0x68068618, ksceKernelLaunchAppPatched);
  n_hooks++;

  hooks[n_hooks++] = taiHookFunctionExportForKernel(KERNEL_PID, &ksceNpDrmGetRifInfoRef, "SceNpDrm",
                                                    0xD84DC44A, 0xDB406EAE, ksceNpDrmGetRifInfoPatched);
  hooks[n_hooks++] = taiHookFunctionExportForKernel(KERNEL_PID, &ksceNpDrmGetRifVitaKeyRef, "SceNpDrm",
                                                    0xD84DC44A, 0x723322B5, ksceNpDrmGetRifVitaKeyPatched);
  hooks[n_hooks++] = taiHookFunctionExportForKernel(KERNEL_PID, &ksceNpDrmGetRifNameRef, "SceNpDrm",
                                                    0xD84DC44A, 0xDF62F3B8, ksceNpDrmGetRifNamePatched);
  hooks[n_hooks++] = taiHookFunctionExportForKernel(KERNEL_PID, &ksceNpDrmGetRifNameForInstallRef, "SceNpDrm",
                                                    0xD84DC44A, 0x17573133, ksceNpDrmGetRifNameForInstallPatched);

  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
  if (hooks[--n_hooks] >= 0)
    taiHookReleaseForKernel(hooks[n_hooks], ksceNpDrmGetRifNameForInstallRef);
  if (hooks[--n_hooks] >= 0)
    taiHookReleaseForKernel(hooks[n_hooks], ksceNpDrmGetRifNameRef);
  if (hooks[--n_hooks] >= 0)
    taiHookReleaseForKernel(hooks[n_hooks], ksceNpDrmGetRifVitaKeyRef);
  if (hooks[--n_hooks] >= 0)
    taiHookReleaseForKernel(hooks[n_hooks], ksceNpDrmGetRifInfoRef);
  if (hooks[--n_hooks] >= 0)
    taiHookReleaseForKernel(hooks[n_hooks], ksceKernelLaunchAppRef);

  return SCE_KERNEL_STOP_SUCCESS;
}
