#ifndef EXYNOS_DISPLAY_MODULE_H
#define EXYNOS_DISPLAY_MODULE_H

#include "ExynosOverlayDisplay.h"

class ExynosPrimaryDisplay : public ExynosOverlayDisplay {
    public:
        ExynosPrimaryDisplay(int numGSCs, struct exynos5_hwc_composer_device_1_t *pdev);
        ~ExynosPrimaryDisplay();
    protected:
        virtual void doPreProcessing(hwc_display_contents_1_t* contents);
        virtual void determineYuvOverlay(hwc_display_contents_1_t *contents);
        virtual bool isOverlaySupported(hwc_layer_1_t &layer, size_t index, bool useVPPOverlay, ExynosMPPModule** supportedInternalMPP, ExynosMPPModule** supportedExternalMPP);
        virtual bool isOverlaySupportedByIDMA(hwc_layer_1_t &layer, size_t index);
        virtual void getIDMAMinSize(hwc_layer_1_t &layer, int *w, int *h);
        ExynosMPPModule          *mCheckIDMAMPP;
};

#endif
