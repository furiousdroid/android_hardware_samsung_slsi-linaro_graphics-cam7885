#ifndef EXYNOS_VPP_VIRTUALDISPLAY_H
#define EXYNOS_VPP_VIRTUALDISPLAY_H

#include "ExynosHWC.h"
#include "ExynosDisplay.h"
#include <videodev2.h>

#define NUM_FRAME_BUFFER 5
#define HWC_SKIP_RENDERING 0x80000000
#define MAX_BUFFER_COUNT 8

#define MAX_VIRTUALDISPLAY_VIDEO_LAYERS 1

class ExynosVirtualDisplay : public ExynosDisplay {
    public:
        /* Methods */
        ExynosVirtualDisplay(struct exynos5_hwc_composer_device_1_t *pdev);
        ~ExynosVirtualDisplay();

        virtual int32_t getDisplayAttributes(const uint32_t attribute, uint32_t config = 0);
        virtual void configureWriteBack(hwc_display_contents_1_t *contents, decon_win_config_data &win_data);

        virtual int blank();
        virtual int unblank();

        virtual int getConfig();

        virtual int prepare(hwc_display_contents_1_t* contents);
        virtual int set(hwc_display_contents_1_t* contents);

        virtual void allocateLayerInfos(hwc_display_contents_1_t* contents);
        virtual void determineYuvOverlay(hwc_display_contents_1_t *contents);
        virtual void determineSupportedOverlays(hwc_display_contents_1_t *contents);
        virtual void determineBandwidthSupport(hwc_display_contents_1_t *contents);

        virtual void init(hwc_display_contents_1_t* contents);
        virtual void deInit();

        int getWFDInfo(int32_t* state, int32_t* compositionType, int32_t* format,
            int64_t* usage, int32_t* width, int32_t* height);

        void setWFDOutputResolution(unsigned int width, unsigned int height, unsigned int disp_w, unsigned int disp_h);
        void setVDSGlesFormat(int format);

        void setPriContents(hwc_display_contents_1_t* contents);

        enum CompositionType {
            COMPOSITION_UNKNOWN = 0,
            COMPOSITION_GLES    = 1,
            COMPOSITION_HWC     = 2,
            COMPOSITION_MIXED   = COMPOSITION_GLES | COMPOSITION_HWC
        };

        unsigned int mWidth;
        unsigned int mHeight;
        unsigned int mDisplayWidth;
        unsigned int mDisplayHeight;

        bool mIsWFDState;

        bool mPresentationMode;
        unsigned int mDeviceOrientation;
        unsigned int mFrameBufferTargetTransform;

        CompositionType mCompositionType;
        CompositionType mPrevCompositionType;
        int mGLESFormat;
        int64_t mSinkUsage;

    protected:
        void setSinkBufferUsage();
        void processGles(hwc_display_contents_1_t* contents);
        void processHwc(hwc_display_contents_1_t* contents);
        void processMixed(hwc_display_contents_1_t* contents);

        virtual void configureHandle(private_handle_t *handle, size_t index, hwc_layer_1_t &layer, int fence_fd, decon_win_config &cfg);
        virtual int postFrame(hwc_display_contents_1_t *contents);
        virtual void determineSkipLayer(hwc_display_contents_1_t *contents);
        virtual bool isSupportGLESformat();
        bool mIsSecureDRM;
        bool mIsNormalDRM;

        hwc_layer_1_t            *mOverlayLayer;
        hwc_layer_1_t            *mFBTargetLayer;
        hwc_layer_1_t            *mFBLayer[NUM_FRAME_BUFFER];
        size_t                   mNumFB;

        void calcDisplayRect(hwc_layer_1_t &layer);

#ifdef USES_DISABLE_COMPOSITIONTYPE_GLES
        ExynosMPPModule *mExternalMPPforCSC;
#endif
};

#endif
