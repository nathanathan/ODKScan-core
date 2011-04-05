package com.opencv.camera;

import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.reflect.Method;
import java.util.Date;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;

import android.content.Context;
import android.graphics.PixelFormat;
import android.hardware.Camera;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.os.Handler;
import android.text.format.Time;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.opencv.camera.NativeProcessor.NativeProcessorCallback;
import com.opencv.camera.NativeProcessor.PoolCallback;

/* NativePreviewer
 * 
 * This class sets up the camera in preview mode and sends the live camera feed
 * to the NativeProcessor for processing. It also handles camera operations such
 * as taking a picture and auto focusing.
 */
public class NativePreviewer extends SurfaceView implements
		SurfaceHolder.Callback, Camera.PreviewCallback, Camera.PictureCallback,
		Camera.AutoFocusCallback, NativeProcessorCallback {

	// Declare a callback used to signal that the picture has been saved
	public interface PictureSavedCallback {
		void OnPictureSaved(String filename);
	}

	// Callback function to call when a picture is saved
	private PictureSavedCallback pictureSavedCallback = null;
	// Handler for sending commands to the UI thread
	private Handler mHandler = new Handler();

	// Camera information
	private boolean mHasAutoFocus = false;
	private AtomicBoolean mIsPreview = new AtomicBoolean(false);
	private SurfaceHolder mHolder;
	private Camera mCamera;
	private int mPreview_width, mPreview_height;
	private int mPixelformat;

	// An instance of NativeProcessor
	private NativeProcessor mProcessor;
	
	// Preview information
	private byte[] mPreviewBuffer = null;
	private Boolean mFnExistSetPreviewCallbackWithBuffer = false;
	private Method mFnAddCallbackBuffer = null;
	private Method mFnSetPreviewCallbackWithBuffer = null;
	private double mFramePerSec = 1;
	
	// Keep track on frame rate
	private Date start = null;
	private long fcount = 0;

	// Keep track of initialization time for perf measurement
	public long initTime;

	private void init() {
		// Install a SurfaceHolder.Callback so we get notified when the
		// underlying surface is created and destroyed.
		mHolder = getHolder();
		mHolder.addCallback(this);
		mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);

		mProcessor = new NativeProcessor();
		setZOrderMediaOverlay(false);

		mFnExistSetPreviewCallbackWithBuffer = false;

		// Check if camera APIs are available
		try {
			mFnSetPreviewCallbackWithBuffer = Class.forName(
					"android.hardware.Camera").getMethod(
					"setPreviewCallbackWithBuffer", PreviewCallback.class);
		} catch (Exception e) {
			Log.e("NativePreviewer",
					"This device does not support setPreviewCallbackWithBuffer.");
			mFnSetPreviewCallbackWithBuffer = null;
		}
		try {
			mFnAddCallbackBuffer = Class.forName("android.hardware.Camera")
					.getMethod("addCallbackBuffer", byte[].class);
		} catch (Exception e) {
			Log.e("NativePreviewer",
					"This device does not support addCallbackBuffer.");
			mFnAddCallbackBuffer = null;
		}
		if (mFnSetPreviewCallbackWithBuffer != null
				&& mFnAddCallbackBuffer != null) {
			mFnExistSetPreviewCallbackWithBuffer = true;
		}
	}

	// Constructor
	public NativePreviewer(Context context, AttributeSet attributes,
			PictureSavedCallback callback) {
		super(context, attributes);

		init();
		pictureSavedCallback = callback;

		/*
		 * TODO get this working! Can't figure out how to define these in xml
		 */
		mPreview_width = attributes.getAttributeIntValue("opencv",
				"preview_width", 600);
		mPreview_height = attributes.getAttributeIntValue("opencv",
				"preview_height", 600);
	}

	// Constructor
	public NativePreviewer(Context context, int preview_width,
			int preview_height, PictureSavedCallback callback) {
		super(context);

		init();
		pictureSavedCallback = callback;

		this.mPreview_width = preview_width;
		this.mPreview_height = preview_height;
	}

	// This function is called by the camera component in the OS when the JPEG
	// image of the picture taken is ready.
	public void onPictureTaken(byte[] data, Camera camera) {
		final String captureImageFolder = "/sdcard/BubbleBot/capturedImages/";
		try {
			Time curTime = new Time();
			curTime.setToNow();

			String filename = curTime.format2445() + ".jpg";

			// Save JPEG image to disk
			FileOutputStream jpgFile = new FileOutputStream(captureImageFolder
					+ filename, false);
			jpgFile.write(data);
			jpgFile.close();
			Log.i("NativePreviewer", "Picture saved to " + captureImageFolder
					+ filename);
			if (pictureSavedCallback != null) {
				pictureSavedCallback.OnPictureSaved(filename);
			}
		} catch (Exception e) {
			Log.e("NativePreviewer", "Failed to save photo", e);
		} finally {
			camera.stopPreview();
		}
	}

	// This function takes a picture with the phone camera
	public void takePicture() {
		Log.i("NativePreviewer", "Taking picture...");
		try {
			// Indicate that we are not in preview mode anymore
			// so we will ignore any autofocus event
			mHandler.removeCallbacks(autofocusrunner);
			mIsPreview.set(false);
			// Clear preview callback. Otherwise, camera will
			// use the preview buffer for saving the photo and it
			// will fail because the buffer is not big enough.
			mCamera.setPreviewCallback(null);
			mProcessor.clearQueue();
			// Set focus mode to auto and flash mode to auto
			Camera.Parameters parameters = mCamera.getParameters();
			if (mHasAutoFocus) {
				Log.i("NativePreviewer", "Enable Auto Focus mode");
				parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
			}
			if (mCamera.getParameters().getSupportedFlashModes()
					.contains(Camera.Parameters.FLASH_MODE_AUTO)) {
				Log.i("NativePreviewer", "Enable Auto Flash mode");
				parameters.setFlashMode(Camera.Parameters.FLASH_MODE_AUTO);
			}
			mCamera.setParameters(parameters);
			// Send auto focus command to camera, when auto focus
			// completes, it will call the onAutoFocus function in
			// this class, which will then take a picture.
			mCamera.autoFocus(this);
		} catch (Exception e) {
			Log.e("NativePreviewer", "takePicture", e);
		}
	}

	// Set preview resolution
	public void setPreviewSize(int width, int height) {
		mPreview_width = width;
		mPreview_height = height;
	}

	// Set camera parameters based on stored preferences 
	public void setParamsFromPrefs(Context ctx) {
		int size[] = { 0, 0 };
		CameraConfig.readImageSize(ctx, size);
		int mode = CameraConfig.readCameraMode(ctx);
		setPreviewSize(size[0], size[1]);
		setGrayscale(mode == CameraConfig.CAMERA_MODE_BW ? true : false);
	}

	// No op. We need to create this function anyway since the class
	// implements SurfaceCallback
	public void surfaceCreated(SurfaceHolder holder) {
	}

	// Release the camera When the surface view is destroyed
	public void surfaceDestroyed(SurfaceHolder holder) {
		releaseCamera();
	}

	// This function is called after the surface is created. It initializes
	// the camera and set up the processor for processing the live camera feed
	public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
		try {
			initCamera(mHolder);
		} catch (InterruptedException e) {
			Log.e("NativePreviewer", "Failed to open camera", e);
			return;
		}

		// Now that the size is known, set up the camera parameters and begin
		// the preview.
		Camera.Parameters parameters = mCamera.getParameters();
		List<Camera.Size> pvsizes = mCamera.getParameters()
				.getSupportedPreviewSizes();
		int best_width = 1000000;
		int best_height = 1000000;
		int bdist = 100000;
		for (Size x : pvsizes) {
			if (Math.abs(x.width - mPreview_width) < bdist) {
				bdist = Math.abs(x.width - mPreview_width);
				best_width = x.width;
				best_height = x.height;
			}
		}
		mPreview_width = best_width;
		mPreview_height = best_height;

		Log.d("NativePreviewer", "Determined compatible preview size is: ("
				+ mPreview_width + "," + mPreview_height + ")");

		// Queries all focus modes
		List<String> fmodes = mCamera.getParameters().getSupportedFocusModes();

		String msg = "";
		for (String mode : fmodes) {
			msg += mode + ";";
		}
		Log.i("NativePreviewer", "Supported Focus modes:" + msg);

		int idx = fmodes.indexOf(Camera.Parameters.FOCUS_MODE_INFINITY);
		if (idx != -1) {
			parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_INFINITY);
		} else if (fmodes.indexOf(Camera.Parameters.FOCUS_MODE_FIXED) != -1) {
			parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_FIXED);
		}

		if (fmodes.indexOf(Camera.Parameters.FOCUS_MODE_AUTO) != -1) {
			mHasAutoFocus = true;
		}

		// Queries all scene modes
		// Interestingly, Nexus 1 does not support any scene modes at all
		List<String> scenemodes = mCamera.getParameters()
				.getSupportedSceneModes();
		msg = "No supported scene modes";
		if (scenemodes != null) {
			msg = "";
			for (String mode : scenemodes) {
				msg += mode + ";";
			}
			if (scenemodes.indexOf(Camera.Parameters.SCENE_MODE_STEADYPHOTO) != -1) {
				parameters
						.setSceneMode(Camera.Parameters.SCENE_MODE_STEADYPHOTO);
				Log.d("NativePreviewer", "***Steady mode");
			}
		}
		Log.i("NativePreviewer", "Supported Scene modes:" + msg);

		parameters.setPictureFormat(PixelFormat.JPEG);
		parameters.setPreviewSize(mPreview_width, mPreview_height);

		// Apply camera settings
		mCamera.setParameters(parameters);

		// Set up the preview callback
		setPreviewCallbackBuffer();
		
		// Start camera preview
		mCamera.startPreview();
		mIsPreview.set(true);
	}

	// This function post a camera auto focus command in the UI thread with
	// the specific delay.
	public void postautofocus(int delay) {
		if (mHasAutoFocus) {
			mHandler.postDelayed(autofocusrunner, delay);
		}
	}

	// This function is called by the camera component in the OS whenever
	// a frame from the live camera feed is ready for processing 
	public void onPreviewFrame(byte[] data, Camera camera) {
		if (start == null) {
			start = new Date();
		}

		// Send the frame to the NativeProcessor for processing.
		// Use the built-in preview buffer if the OS supports previewing with buffer.
		// Otherwise, allocate memory for the buffer and copy the frame to the buffer.
		if (mProcessor.isActive()) {
			if (mFnExistSetPreviewCallbackWithBuffer) {
				mProcessor.post(data, mPreview_width, mPreview_height,
						mPixelformat, System.nanoTime(), this);
			} else {
				System.arraycopy(data, 0, mPreviewBuffer, 0, data.length);
				mProcessor.post(mPreviewBuffer, mPreview_width,
						mPreview_height, mPixelformat, System.nanoTime(), this);
			}
		} else {
			Log.i("NativePreviewer",
					"Ignoring preview frame since processor is inactive.");
		}

		// Calculate frames per second
		fcount++;
		if (fcount % 10 == 0) {
			double ms = (new Date()).getTime() - start.getTime();
			mFramePerSec = (mFramePerSec + (fcount / (ms / 1000.0))) / 2;
			Log.i("NativePreviewer", "fps:" + mFramePerSec);
			start = new Date();
			fcount = 0;
		}
	}

	// This function returns the nanoseconds needed to preview one frame.
	// It is called by BubbleCollect for calibrating the auto focus interval.
	public long getNanoSecondsPerFrame() {
		return Math.round(1000000000L / mFramePerSec);
	}

	// This function is called when the NativeProcessor has completed a frame.
	// If the preview is done using the built-in buffer, it will reuse that buffer.
	public void onDoneNativeProcessing(byte[] buffer) {
		if (mFnExistSetPreviewCallbackWithBuffer) {
			try {
				mFnAddCallbackBuffer.invoke(mCamera, buffer);
			} catch (Exception e) {
				Log.e("NativePreviewer", "Invoking addCallbackBuffer failed: "
						+ e.toString());
			}
		}
	}

	// This function is used by other activity to register a list of ordered callbacks
	// that the NativeProcessor will call for each preview frame
	public void addCallbackStack(LinkedList<PoolCallback> callbackstack) {
		mProcessor.addCallbackStack(callbackstack);
	}

	// When the activity pauses, stop camera preview and release the buffers.
	// Also, release the camera so other application can use it.
	public void onPause() {
		try {
			mIsPreview.set(false);
			mCamera.setPreviewCallback(null);
			mProcessor.clearQueue();
			mCamera.stopPreview();
			addCallbackStack(null);
		} catch (Exception e) {
			Log.e("NativePreviewer", "Error during onPause()", e);
		} finally {
			releaseCamera();
			mProcessor.stop();
		}
	}

	// When the activity resumes, set up the preview buffer and start previewing
	public void onResume() {
		try {
			initCamera(mHolder);
		} catch (InterruptedException e) {
			Log.e("NativePreviewer", "Failed to open camera", e);
			return;
		}
		mProcessor.start();
		setPreviewCallbackBuffer();
		mCamera.startPreview();
		mIsPreview.set(true);
	}

	// This function sets up the preview callback
	private void setPreviewCallbackBuffer() {
		// Calculate the size of the preview buffer
		PixelFormat pixelinfo = new PixelFormat();
		mPixelformat = mCamera.getParameters().getPreviewFormat();
		PixelFormat.getPixelFormatInfo(mPixelformat, pixelinfo);

		Size previewSize = mCamera.getParameters().getPreviewSize();
		mPreviewBuffer = new byte[previewSize.width * previewSize.height
				* pixelinfo.bitsPerPixel / 8];

		// If the OS supports previewing with buffer (API level 8),
		// set up the preview using the built-in buffer. Otherwise,
		// just set up preview with no buffer - we will allocate buffer
		// on the fly.
		if (mFnExistSetPreviewCallbackWithBuffer) {
			try {
				mFnAddCallbackBuffer.invoke(mCamera, mPreviewBuffer);
			} catch (Exception e) {
				Log.e("NativePreviewer", "Invoking addCallbackBuffer failed: "
						+ e.toString());
			}
			try {
				mFnSetPreviewCallbackWithBuffer.invoke(mCamera, this);
			} catch (Exception e) {
				Log.e("NativePreviewer",
						"Invoking setPreviewCallbackWithBuffer failed: "
								+ e.toString());
			}
		} else {
			mCamera.setPreviewCallback(this);
		}
	}

	// This task is used to sends an auto focus command to the camera
	private Runnable autofocusrunner = new Runnable() {
		public void run() {
			if (mIsPreview.get()) {
				Log.i("NativePreviewer", "autofocusing...");
				mCamera.autoFocus(autocallback);
			}
		}
	};

	// This callback function is called by the OS when an auto focus
	// command is completed. It will retry auto focusing in 1 second
	// if the auto focus failed.
	private Camera.AutoFocusCallback autocallback = new Camera.AutoFocusCallback() {
		public void onAutoFocus(boolean success, Camera camera) {
			if (!success) {
				Log.e("NativePreviewer", "autofocus failed");
				postautofocus(1000);
			}
		}
	};

	// This function initializes the camera.
	private void initCamera(SurfaceHolder holder) throws InterruptedException {
		if (mCamera == null) {
			for (int i = 0; i < 10; i++)
				try {
					mCamera = Camera.open();
					break;
				} catch (RuntimeException e) {
					Log.e("NativePreviewer", "Failed to open camera", e);
					Thread.sleep(200);
					if (i == 9) {
						throw e;
					}
				}
		}
		try {
			mCamera.setPreviewDisplay(holder);
		} catch (IOException exception) {
			mCamera.release();
			mCamera = null;
		} catch (RuntimeException e) {
			Log.e("camera", "stacktrace", e);
		}
	}

	// This function releases the camera
	private void releaseCamera() {
		if (mCamera != null) {
			// Surface will be destroyed when we return, so stop the preview.
			// Because the CameraDevice object is not a shared resource, it's
			// very
			// important to release it when the activity is paused.
			mCamera.stopPreview();
			mCamera.release();
		}

		mCamera = null;
	}

	// This function instructs the NativeProcessor to operate in color or B&W mode
	public void setGrayscale(boolean b) {
		mProcessor.setGrayscale(b);
	}

	// This callback function is called by the OS when NativePreviewer issues an
	// auto focus command. It will then take a picture. This is different from the
	// autocallback defined earlier in this class, which is used for general purpose
	// auto focusing during the camera preview.
	public void onAutoFocus(boolean success, Camera camera) {
		Log.i("NativePreviewer", "Auto focus "
				+ ((success) ? "succeeded" : "failed") + ". Taking picture...");
		camera.takePicture(null, null, this);
	}
}
