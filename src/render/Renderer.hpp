#pragma once

#include "../defines.hpp"
#include <list>
#include "../helpers/Monitor.hpp"
#include "OpenGL.hpp"
#include "Renderbuffer.hpp"
#include "../helpers/Timer.hpp"
#include "../helpers/Region.hpp"

struct SMonitorRule;
class CWorkspace;
class CWindow;
class CInputPopup;
class IWLBuffer;

// TODO: add fuller damage tracking for updating only parts of a window
enum DAMAGETRACKINGMODES {
    DAMAGE_TRACKING_INVALID = -1,
    DAMAGE_TRACKING_NONE    = 0,
    DAMAGE_TRACKING_MONITOR,
    DAMAGE_TRACKING_FULL
};

enum eRenderPassMode {
    RENDER_PASS_ALL = 0,
    RENDER_PASS_MAIN,
    RENDER_PASS_POPUP
};

enum eRenderMode {
    RENDER_MODE_NORMAL              = 0,
    RENDER_MODE_FULL_FAKE           = 1,
    RENDER_MODE_TO_BUFFER           = 2,
    RENDER_MODE_TO_BUFFER_READ_ONLY = 3,
};

class CToplevelExportProtocolManager;
class CInputManager;
struct SSessionLockSurface;

class CHyprRenderer {
  public:
    CHyprRenderer();

    void                            renderMonitor(PHLMONITOR pMonitor);
    void                            arrangeLayersForMonitor(const int&);
    void                            damageSurface(SP<CWLSurfaceResource>, double, double, double scale = 1.0);
    void                            damageWindow(PHLWINDOW, bool forceFull = false);
    void                            damageBox(CBox*);
    void                            damageBox(const int& x, const int& y, const int& w, const int& h);
    void                            damageRegion(const CRegion&);
    void                            damageMonitor(PHLMONITOR);
    void                            damageMirrorsWith(PHLMONITOR, const CRegion&);
    bool                            applyMonitorRule(PHLMONITOR, SMonitorRule*, bool force = false);
    bool                            shouldRenderWindow(PHLWINDOW, PHLMONITOR);
    bool                            shouldRenderWindow(PHLWINDOW);
    void                            ensureCursorRenderingMode();
    bool                            shouldRenderCursor();
    void                            setCursorHidden(bool hide);
    void                            calculateUVForSurface(PHLWINDOW, SP<CWLSurfaceResource>, bool main = false, const Vector2D& projSize = {}, bool fixMisalignedFSV1 = false);
    std::tuple<float, float, float> getRenderTimes(PHLMONITOR pMonitor); // avg max min
    void                            renderLockscreen(PHLMONITOR pMonitor, timespec* now, const CBox& geometry);
    void                            setOccludedForBackLayers(CRegion& region, PHLWORKSPACE pWorkspace);
    void                            setOccludedForMainWorkspace(CRegion& region, PHLWORKSPACE pWorkspace); // TODO: merge occlusion methods
    bool                            canSkipBackBufferClear(PHLMONITOR pMonitor);
    void                            recheckSolitaryForMonitor(PHLMONITOR pMonitor);
    void                            setCursorSurface(SP<CWLSurface> surf, int hotspotX, int hotspotY, bool force = false);
    void                            setCursorFromName(const std::string& name, bool force = false);
    void                            onRenderbufferDestroy(CRenderbuffer* rb);
    CRenderbuffer*                  getCurrentRBO();
    bool                            isNvidia();
    void                            makeEGLCurrent();
    void                            unsetEGL();

    // if RENDER_MODE_NORMAL, provided damage will be written to.
    // otherwise, it will be the one used.
    bool beginRender(PHLMONITOR pMonitor, CRegion& damage, eRenderMode mode = RENDER_MODE_NORMAL, SP<IWLBuffer> buffer = {}, CFramebuffer* fb = nullptr, bool simple = false);
    void endRender();

    bool m_bBlockSurfaceFeedback = false;
    bool m_bRenderingSnapshot    = false;
    PHLWINDOWREF  m_pLastScanout;
    PHLMONITORREF m_pMostHzMonitor;
    bool          m_bDirectScanoutBlocked = false;

    DAMAGETRACKINGMODES
    damageTrackingModeFromStr(const std::string&);

    bool             attemptDirectScanout(PHLMONITOR);
    void             setWindowScanoutMode(PHLWINDOW);
    void             initiateManualCrash();

    bool             m_bCrashingInProgress = false;
    float            m_fCrashingDistort    = 0.5f;
    wl_event_source* m_pCrashingLoop       = nullptr;
    wl_event_source* m_pCursorTicker       = nullptr;

    CTimer           m_tRenderTimer;

    struct {
        int                           hotspotX;
        int                           hotspotY;
        std::optional<SP<CWLSurface>> surf;
        std::string                   name;
    } m_sLastCursorData;

  private:
    void           arrangeLayerArray(PHLMONITOR, const std::vector<PHLLSREF>&, bool, CBox*);
    void           renderWorkspaceWindowsFullscreen(PHLMONITOR, PHLWORKSPACE, timespec*); // renders workspace windows (fullscreen) (tiled, floating, pinned, but no special)
    void           renderWorkspaceWindows(PHLMONITOR, PHLWORKSPACE, timespec*);           // renders workspace windows (no fullscreen) (tiled, floating, pinned, but no special)
    void           renderWindow(PHLWINDOW, PHLMONITOR, timespec*, bool, eRenderPassMode, bool ignorePosition = false, bool ignoreAllGeometry = false);
    void           renderLayer(PHLLS, PHLMONITOR, timespec*, bool popups = false);
    void           renderSessionLockSurface(SSessionLockSurface*, PHLMONITOR, timespec*);
    void           renderDragIcon(PHLMONITOR, timespec*);
    void           renderIMEPopup(CInputPopup*, PHLMONITOR, timespec*);
    void           renderWorkspace(PHLMONITOR pMonitor, PHLWORKSPACE pWorkspace, timespec* now, const CBox& geometry);
    void           sendFrameEventsToWorkspace(PHLMONITOR pMonitor, PHLWORKSPACE pWorkspace, timespec* now); // sends frame displayed events but doesn't actually render anything
    void           renderAllClientsForWorkspace(PHLMONITOR pMonitor, PHLWORKSPACE pWorkspace, timespec* now, const Vector2D& translate = {0, 0}, const float& scale = 1.f);

    bool           m_bCursorHidden        = false;
    bool           m_bCursorHasSurface    = false;
    CRenderbuffer* m_pCurrentRenderbuffer = nullptr;
    wlr_buffer*    m_pCurrentWlrBuffer    = nullptr;
    WP<IWLBuffer>  m_pCurrentHLBuffer     = {};
    eRenderMode    m_eRenderMode          = RENDER_MODE_NORMAL;

    bool           m_bNvidia = false;

    struct {
        bool hiddenOnTouch    = false;
        bool hiddenOnTimeout  = false;
        bool hiddenOnKeyboard = false;
    } m_sCursorHiddenConditions;

    CRenderbuffer*                              getOrCreateRenderbuffer(wlr_buffer* buffer, uint32_t fmt);
    CRenderbuffer*                              getOrCreateRenderbuffer(SP<IWLBuffer> buffer, uint32_t fmt);
    std::vector<std::unique_ptr<CRenderbuffer>> m_vRenderbuffers;

    friend class CHyprOpenGLImpl;
    friend class CToplevelExportProtocolManager;
    friend class CInputManager;
    friend class CPointerManager;
};

inline std::unique_ptr<CHyprRenderer> g_pHyprRenderer;
