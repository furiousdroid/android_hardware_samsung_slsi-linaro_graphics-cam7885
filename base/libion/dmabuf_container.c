/*
 *  dmabuf_container.c
 *
 *   Copyright 2018 Samsung Electronics Co., Ltd.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#define LOG_TAG "dmabuf-container"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <string.h>

#include <sys/ioctl.h>

#include <log/log.h>

#include <hardware/exynos/ion.h>
#include <hardware/exynos/dmabuf_container.h>

struct dma_buf_merge {
    int      *dma_bufs;
    int32_t  count;
    int32_t  dmabuf_container;
    uint32_t reserved[2];
};
#define DMA_BUF_BASE        'b'
#define DMA_BUF_IOCTL_MERGE                 _IOWR(DMA_BUF_BASE, 13, struct dma_buf_merge)
#define DMA_BUF_IOCTL_CONTAINER_SET_MASK    _IOW(DMA_BUF_BASE, 14, __u32)
#define DMA_BUF_IOCTL_CONTAINER_GET_MASK    _IOR(DMA_BUF_BASE, 14, __u32)

struct dma_buf_mask {
    int     dmabuf_container;
    __u32   reserved;
    __u32   mask[2];
};

#define DMABUF_CONTAINER_BASE        'D'
#define DMABUF_CONTAINER_IOCTL_MERGE    _IOWR(DMABUF_CONTAINER_BASE, 1, struct dma_buf_merge)
#define DMABUF_CONTAINER_IOCTL_SET_MASK _IOW(DMABUF_CONTAINER_BASE, 3, struct dma_buf_mask)
#define DMABUF_CONTAINER_IOCTL_GET_MASK _IOR(DMABUF_CONTAINER_BASE, 4, struct dma_buf_mask)

#define DMABUF_CONTAINER_NAME "/dev/dmabuf_container"

#define MAX_DMABUF_CONTAINER_BUFS   64

int dma_buf_merge(int base_fd, int src_fds[], int src_count)
{
    int ret, fd = open(DMABUF_CONTAINER_NAME, O_RDONLY | O_CLOEXEC);
    struct dma_buf_merge data;

    if (fd >= 0) { /* interface dmabuf container */
        data.count = src_count + 1;
        if (data.count > MAX_DMABUF_CONTAINER_BUFS) {
            close(fd);
            return -EINVAL;
        }

        data.dma_bufs = (int32_t *)calloc(data.count, sizeof(int32_t));
        if (!data.dma_bufs) {
            close(fd);
            return -ENOMEM;
        }

        data.dma_bufs[0] = base_fd;
        memcpy(&data.dma_bufs[1], src_fds, sizeof(int32_t) * src_count);

        ret = ioctl(fd, DMABUF_CONTAINER_IOCTL_MERGE, &data);
        free(data.dma_bufs);

        close(fd);
    } else { /* interface dmabuf */
        data.dma_bufs = src_fds;
        data.count = src_count;

        ret = ioctl(base_fd, DMA_BUF_IOCTL_MERGE, &data);
    }

    if (ret < 0) {
        ALOGE("failed to merge %d dma-bufs: %s", src_count, strerror(errno));
        return ret;
    }

    return data.dmabuf_container;
}

int dmabuf_container_set_mask64(int dmabuf, uint64_t mask)
{
    int ret, fd = open(DMABUF_CONTAINER_NAME, O_RDONLY | O_CLOEXEC);

    if (fd >= 0) {
        struct dma_buf_mask data;

        data.dmabuf_container = dmabuf;
        data.mask[0] = (mask << 32) >> 32;
        data.mask[1] = mask >> 32;

        ret = ioctl(fd, DMABUF_CONTAINER_IOCTL_SET_MASK, &data);

        close(fd);
    } else {
        uint32_t data = (mask << 32) >> 32;

        ret = ioctl(dmabuf, DMA_BUF_IOCTL_CONTAINER_SET_MASK, &data);
    }

    if (ret < 0) {
            ALOGE("Failed to configure dma-buf container mask %#lx: %s", (unsigned long)mask, strerror(errno));
            return ret;
    }

    return 0;

}

int dmabuf_container_get_mask64(int dmabuf, uint64_t *mask)
{
    int ret, fd = open(DMABUF_CONTAINER_NAME, O_RDONLY | O_CLOEXEC);

    if (fd >= 0) {
        struct dma_buf_mask data;

        data.dmabuf_container = dmabuf;

        ret = ioctl(fd, DMABUF_CONTAINER_IOCTL_GET_MASK, &data);

        *mask = ((uint64_t)data.mask[1] << 32) | data.mask[0];

        close(fd);
    } else {
        uint32_t data;

        ret = ioctl(dmabuf, DMA_BUF_IOCTL_CONTAINER_GET_MASK, &data);

        *mask = data;
    }

    if (ret < 0) {
        ALOGE("Failed to retrieve dma-buf container mask: %s", strerror(errno));
        return ret;
    }

    return 0;
}

int dmabuf_container_get_mask(int dmabuf, uint32_t *mask)
{
    uint64_t data;
    int ret;

    ret = dmabuf_container_get_mask64(dmabuf, &data);
    *mask = (uint32_t)data;

    return ret;
}

int dmabuf_container_set_mask(int dmabuf, uint32_t mask)
{
    return dmabuf_container_set_mask64(dmabuf, (uint64_t)mask);
}
