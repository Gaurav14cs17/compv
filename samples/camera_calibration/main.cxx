#include <compv/compv_api.h>

using namespace compv;

#define CAMERA_IDX			0
#define CAMERA_WIDTH		1280
#define CAMERA_HEIGHT		720
#define CAMERA_FPS			25
#define CAMERA_SUBTYPE		COMPV_SUBTYPE_PIXELS_YUY2
#define CAMERA_AUTOFOCUS	true

#define WINDOW_WIDTH		1280
#define WINDOW_HEIGHT		720

#define TAG_SAMPLE			"Camera calibration"

/* My runloop listener (optional) */
COMPV_OBJECT_DECLARE_PTRS(MyRunLoopListener)
class CompVMyRunLoopListener : public CompVRunLoopListener
{
protected:
	CompVMyRunLoopListener(CompVCameraPtr camera, const std::string& cameraId) : m_ptrCamera(camera), m_strCameraId(cameraId) { }
public:
	virtual ~CompVMyRunLoopListener() { }
	virtual COMPV_ERROR_CODE onStateChanged(COMPV_RUNLOOP_STATE newState) override {
		COMPV_DEBUG_INFO_EX(TAG_SAMPLE, "RunLoop onStateChanged(%d)", newState);
		switch (newState) {
		case COMPV_RUNLOOP_STATE_LOOP_STARTED:
		default:
			return COMPV_ERROR_CODE_S_OK;
		case COMPV_RUNLOOP_STATE_ANIMATION_STARTED:
		case COMPV_RUNLOOP_STATE_ANIMATION_RESUMED:
			return m_ptrCamera->start(m_strCameraId);
		case COMPV_RUNLOOP_STATE_ANIMATION_PAUSED:
		case COMPV_RUNLOOP_STATE_LOOP_STOPPED:
		case COMPV_RUNLOOP_STATE_ANIMATION_STOPPED:
			return m_ptrCamera->stop();
		}
	}
	static COMPV_ERROR_CODE newObj(CompVMyRunLoopListenerPtrPtr listener, CompVCameraPtr camera, const std::string& cameraId) {
		COMPV_CHECK_EXP_RETURN(!listener || !camera, COMPV_ERROR_CODE_E_INVALID_PARAMETER);
		*listener = new CompVMyRunLoopListener(camera, cameraId);
		COMPV_CHECK_EXP_RETURN(!*listener, COMPV_ERROR_CODE_E_OUT_OF_MEMORY);
		return COMPV_ERROR_CODE_S_OK;
	}
private:
	CompVCameraPtr m_ptrCamera;
	const std::string m_strCameraId;
};

/* My camera listener */
COMPV_OBJECT_DECLARE_PTRS(MyCameraListener)
class CompVMyCameraListener : public CompVCameraListener
{
protected:
	CompVMyCameraListener(CompVWindowPtr ptrWindow, CompVSingleSurfaceLayerPtr ptrSingleSurfaceLayer)
		: m_ptrWindow(ptrWindow), m_ptrSingleSurfaceLayer(ptrSingleSurfaceLayer) { }
public:
	virtual ~CompVMyCameraListener() { }

	virtual COMPV_ERROR_CODE onNewFrame(const CompVMatPtr& image) override {
		COMPV_ERROR_CODE err = COMPV_ERROR_CODE_S_OK;
		if (CompVDrawing::isLoopRunning()) {
			CompVMatPtr imageGray;
			CompVMatPtr points;
#if 0
			COMPV_CHECK_CODE_RETURN(CompVImage::convertGrayscale(image, &imageGray));
#else
			size_t file_index = 55; // 47 + (rand() % 20); //47 + (rand() % 20);
			std::string file_path = std::string("C:/Projects/GitHub/data/calib/P10100")+ CompVBase::to_string(file_index) +std::string("s_640x480_gray.yuv");
			COMPV_CHECK_CODE_RETURN(CompVImage::readPixels(COMPV_SUBTYPE_PIXELS_Y, 640, 480, 640, file_path.c_str(), &imageGray));
			CompVThread::sleep(1000);
#endif
			COMPV_CHECK_CODE_RETURN(m_ptrCalib->process(imageGray, m_CalibResult));

			if (!m_CalibResult.hough_lines.empty()) {
				COMPV_CHECK_CODE_RETURN(buildLines(m_CalibResult.edges->cols(), m_CalibResult.edges->rows(), m_CalibResult.hough_lines, points));
			}
			COMPV_CHECK_CODE_BAIL(err = m_ptrWindow->beginDraw());
			COMPV_CHECK_CODE_BAIL(err = m_ptrSingleSurfaceLayer->surface()->drawImage(m_CalibResult.edges));
			if (points) {
				COMPV_CHECK_CODE_BAIL(err = m_ptrSingleSurfaceLayer->surface()->renderer()->canvas()->drawLines(
					points->ptr<const compv_float32_t>(0), points->ptr<const compv_float32_t>(1),
					points->ptr<const compv_float32_t>(2), points->ptr<const compv_float32_t>(3),
					m_CalibResult.hough_lines.size(), &m_DrawingOptions
				));
			}
			COMPV_CHECK_CODE_BAIL(err = m_ptrSingleSurfaceLayer->blit());
		bail:
			COMPV_CHECK_CODE_NOP(err = m_ptrWindow->endDraw()); // Make sure 'endDraw()' will be called regardless the result
		}
		return err;
	}

	virtual COMPV_ERROR_CODE onError(const std::string& message) override {
		COMPV_DEBUG_ERROR_EX(TAG_SAMPLE, "Camera error: %s", message.c_str()); // probably a disconnect
		return COMPV_ERROR_CODE_S_OK;
	}

	static COMPV_ERROR_CODE newObj(CompVMyCameraListenerPtrPtr listener, CompVWindowPtr ptrWindow, CompVSingleSurfaceLayerPtr ptrSingleSurfaceLayer) {
		COMPV_CHECK_EXP_RETURN(!listener || !ptrWindow || !ptrSingleSurfaceLayer, COMPV_ERROR_CODE_E_INVALID_PARAMETER);
		CompVMyCameraListenerPtr listener_ = new CompVMyCameraListener(ptrWindow, ptrSingleSurfaceLayer);
		COMPV_CHECK_EXP_RETURN(!listener_, COMPV_ERROR_CODE_E_OUT_OF_MEMORY);
		COMPV_CHECK_CODE_RETURN(CompVCalibCamera::newObj(&listener_->m_ptrCalib));
		listener_->m_DrawingOptions.colorType = COMPV_DRAWING_COLOR_TYPE_STATIC;
		listener_->m_DrawingOptions.color[0] = 1.f;
		listener_->m_DrawingOptions.color[1] = 1.f;
		listener_->m_DrawingOptions.color[2] = 0.f;
		listener_->m_DrawingOptions.color[3] = 1.f;
		listener_->m_DrawingOptions.lineWidth = 1.5f;

		*listener = listener_;
		return COMPV_ERROR_CODE_S_OK;
	}

private:
	static COMPV_ERROR_CODE buildLines(size_t imageWidth, size_t imageHeight, const CompVHoughLineVector& lines, CompVMatPtr& points) {
		COMPV_CHECK_EXP_RETURN(lines.empty(), COMPV_ERROR_CODE_E_INVALID_PARAMETER);
		COMPV_CHECK_CODE_RETURN((CompVMat::newObjAligned<compv_float32_t>(&points, 4, lines.size())));
		compv_float32_t *px0 = points->ptr<compv_float32_t>(0);
		compv_float32_t *py0 = points->ptr<compv_float32_t>(1);
		compv_float32_t *px1 = points->ptr<compv_float32_t>(2);
		compv_float32_t *py1 = points->ptr<compv_float32_t>(3);
		const compv_float32_t imageWidthF = static_cast<compv_float32_t>(imageWidth);
		const compv_float32_t imageHeightF = static_cast<compv_float32_t>(imageHeight);
		const compv_float32_t half_imageWidthF = imageWidthF / 2.f;
		const compv_float32_t half_imageHeightF = imageHeightF / 2.f;
		for (size_t i = 0; i < lines.size(); i++) {
			const compv_float32_t rho = lines[i].rho;
			const compv_float32_t theta = lines[i].theta;
			const compv_float32_t a = std::cos(theta), b = 1.f / std::sin(theta);
			px0[i] = 0;
			py0[i] = ((rho + (half_imageWidthF * a)) * b) + half_imageHeightF;
			px1[i] = imageWidthF;
			py1[i] = ((rho - (half_imageWidthF * a)) * b) + half_imageHeightF;
		}
		return COMPV_ERROR_CODE_S_OK;
	}

private:
	CompVWindowPtr m_ptrWindow;
	CompVSingleSurfaceLayerPtr m_ptrSingleSurfaceLayer;
	CompVCalibCameraPtr m_ptrCalib;
	CompVCalibCameraResult m_CalibResult;
	CompVDrawingOptions m_DrawingOptions;
};

/* Entry point function */
compv_main()
{
	{
		COMPV_ERROR_CODE err;
		CompVWindowPtr window;
		CompVMyRunLoopListenerPtr runloopListener;
		CompVSingleSurfaceLayerPtr singleSurfaceLayer;
		CompVCameraPtr camera;
		CompVMyCameraListenerPtr cameraListener;
		CompVCameraDeviceInfoList devices;
		std::string cameraId = ""; // empty string means default

								   // Change debug level to INFO before starting
		CompVDebugMgr::setLevel(COMPV_DEBUG_LEVEL_INFO);

		// Init the modules
		COMPV_CHECK_CODE_BAIL(err = CompVInit());

		// Create "Hello world!" window and add a surface for drawing
		COMPV_CHECK_CODE_BAIL(err = CompVWindow::newObj(&window, WINDOW_WIDTH, WINDOW_HEIGHT, TAG_SAMPLE));
		COMPV_CHECK_CODE_BAIL(err = window->addSingleLayerSurface(&singleSurfaceLayer));

		// Create my camera and add a listener to it 
		COMPV_CHECK_CODE_BAIL(err = CompVCamera::newObj(&camera));

		COMPV_CHECK_CODE_BAIL(err = CompVMyCameraListener::newObj(&cameraListener, window, *singleSurfaceLayer));
		COMPV_CHECK_CODE_BAIL(err = camera->setListener(*cameraListener));
		// Get list of devices/cameras and print them to the screen (optional)
		COMPV_CHECK_CODE_BAIL(err = camera->devices(devices));
		COMPV_CHECK_EXP_BAIL(devices.empty(), err = COMPV_ERROR_CODE_E_NOT_FOUND, "No camera device found");
		for (CompVCameraDeviceInfoList::iterator it = devices.begin(); it != devices.end(); ++it) {
			COMPV_DEBUG_INFO_EX(TAG_SAMPLE, "Camera device: %s -> %s, %s", it->id.c_str(), it->name.c_str(), it->description.c_str());
		}
		// Set camera parameters (optional)
		cameraId = devices[COMPV_MATH_MIN(CAMERA_IDX, (devices.size() - 1))].id;
		COMPV_CHECK_CODE_BAIL(err = camera->setInt(COMPV_CAMERA_CAP_INT_WIDTH, CAMERA_WIDTH));
		COMPV_CHECK_CODE_BAIL(err = camera->setInt(COMPV_CAMERA_CAP_INT_HEIGHT, CAMERA_HEIGHT));
		COMPV_CHECK_CODE_BAIL(err = camera->setInt(COMPV_CAMERA_CAP_INT_FPS, CAMERA_FPS));
		COMPV_CHECK_CODE_BAIL(err = camera->setInt(COMPV_CAMERA_CAP_INT_SUBTYPE, CAMERA_SUBTYPE));
		COMPV_CHECK_CODE_BAIL(err = camera->setBool(COMPV_CAMERA_CAP_BOOL_AUTOFOCUS, CAMERA_AUTOFOCUS));
		COMPV_CHECK_CODE_BAIL(err = camera->start(cameraId)); // use no parameter ('star()') to use default camera device

															  // Start ui runloop
															  // Setting a listener is optional but used here to show how to handle Android onStart, onPause, onResume.... activity states
		COMPV_CHECK_CODE_BAIL(err = CompVMyRunLoopListener::newObj(&runloopListener, camera, cameraId));
		COMPV_CHECK_CODE_BAIL(err = CompVDrawing::runLoop(*runloopListener));

	bail:
		if (COMPV_ERROR_CODE_IS_NOK(err)) {
			COMPV_DEBUG_ERROR_EX(TAG_SAMPLE, "Something went wrong!!");
		}
	}

	COMPV_DEBUG_CHECK_FOR_MEMORY_LEAKS();

	compv_main_return(0);
}