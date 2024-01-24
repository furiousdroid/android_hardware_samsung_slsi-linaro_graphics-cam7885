#ifndef EXYNOS_GSC_MODULE_H
#define EXYNOS_GSC_MODULE_H

#include "ExynosMPPv2.h"

class ExynosDisplay;

enum {
    eMPPRot1    =       1,
    eMPPRot2    =       2,
    eMPPRot3    =       3,
    eMPPRot4    =       4,
    eMPPRot5    =       5,
};

class ExynosMPPModule : public ExynosMPP {
    public:
        ExynosMPPModule();
        ExynosMPPModule(ExynosDisplay *display, unsigned int mppType, unsigned int mppIndex);
        virtual int getMaxDownscale();
        virtual int getMaxUpscale();
        virtual int getMaxDownscale(hwc_layer_1_t &layer);
        virtual int getDstWidthAlign(int format);
        virtual int getDstHeightAlign(int format);
        virtual int getSrcXOffsetAlign(hwc_layer_1_t &layer);
        virtual int getSrcYOffsetAlign(hwc_layer_1_t &layer);
        virtual int getCropWidthAlign(hwc_layer_1_t &layer);
        virtual int getCropHeightAlign(hwc_layer_1_t &layer);
        virtual int getMaxWidth(hwc_layer_1_t &layer);
        virtual int getMaxHeight(hwc_layer_1_t &layer);
        virtual int getMinWidth(hwc_layer_1_t &layer);
        virtual int getMinHeight(hwc_layer_1_t &layer);
        virtual int getSrcWidthAlign(hwc_layer_1_t &layer);
        virtual int getSrcHeightAlign(hwc_layer_1_t &layer);
        virtual int getMaxCropWidth(hwc_layer_1_t &layer);
        virtual int getMaxCropHeight(hwc_layer_1_t &layer);
        virtual bool isFormatSupportedByMPP(int format);
    protected:
        virtual int getMinCropWidth(hwc_layer_1_t &layer);
        virtual int getMinCropHeight(hwc_layer_1_t &layer);
        virtual int getMaxDstWidth(int format);
        virtual int getMaxDstHeight(int format);
        virtual int getMinDstWidth(int format);
        virtual int getMinDstHeight(int format);
        virtual int getBufferUsage(private_handle_t *srcHandle);
    private:
        bool checkRotationCase(hwc_layer_1_t &layer, uint8_t rotType);
};

#endif
