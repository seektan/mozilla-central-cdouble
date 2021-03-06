/* -*- Mode: Java; c-basic-offset: 4; tab-width: 20; indent-tabs-mode: nil; -*-
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Android code.
 *
 * The Initial Developer of the Original Code is Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2009-2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Patrick Walton <pcwalton@mozilla.com>
 *   Chris Lord <chrislord.net@gmail.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.CairoImage;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerClient;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.LayerRenderer;
import org.mozilla.gecko.gfx.PointUtils;
import org.mozilla.gecko.gfx.SingleTileLayer;
import org.mozilla.gecko.gfx.WidgetTileLayer;
import org.mozilla.gecko.FloatUtils;
import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoEventListener;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.DisplayMetrics;
import android.util.Log;
import org.json.JSONException;
import org.json.JSONObject;
import java.nio.ByteBuffer;

/**
 * Transfers a software-rendered Gecko to an ImageLayer so that it can be rendered by our
 * compositor.
 *
 * TODO: Throttle down Gecko's priority when we pan and zoom.
 */
public class GeckoSoftwareLayerClient extends LayerClient implements GeckoEventListener {
    private static final String LOGTAG = "GeckoSoftwareLayerClient";

    private Context mContext;
    private int mFormat;
    private IntSize mScreenSize, mViewportSize;
    private IntSize mBufferSize;
    private ByteBuffer mBuffer;
    private Layer mTileLayer;

    /* The viewport rect that Gecko is currently displaying. */
    private ViewportMetrics mGeckoViewport;

    private CairoImage mCairoImage;

    private static final long MIN_VIEWPORT_CHANGE_DELAY = 350L;
    private long mLastViewportChangeTime;
    private boolean mPendingViewportAdjust;
    private boolean mViewportSizeChanged;

    // mUpdateViewportOnEndDraw is used to indicate that we received a
    // viewport update notification while drawing. therefore, when the
    // draw finishes, we need to update the entire viewport rather than
    // just the page size. this boolean should always be accessed from
    // inside a transaction, so no synchronization is needed.
    private boolean mUpdateViewportOnEndDraw;

    public GeckoSoftwareLayerClient(Context context) {
        mContext = context;

        mScreenSize = new IntSize(0, 0);
        mBufferSize = new IntSize(0, 0);
        mFormat = CairoImage.FORMAT_RGB16_565;

        mCairoImage = new CairoImage() {
            @Override
            public ByteBuffer getBuffer() { return mBuffer; }
            @Override
            public IntSize getSize() { return mBufferSize; }
            @Override
            public int getFormat() { return mFormat; }
        };

        mTileLayer = new SingleTileLayer(mCairoImage);
    }


    protected void finalize() throws Throwable {
        try {
            if (mBuffer != null)
                GeckoAppShell.freeDirectBuffer(mBuffer);
            mBuffer = null;
        } finally {
            super.finalize();
        }
    }

    public void installWidgetLayer() {
        mTileLayer = new WidgetTileLayer(mCairoImage);
    }

    /** Attaches the root layer to the layer controller so that Gecko appears. */
    @Override
    public void setLayerController(LayerController layerController) {
        super.setLayerController(layerController);

        layerController.setRoot(mTileLayer);
        if (mGeckoViewport != null) {
            layerController.setViewportMetrics(mGeckoViewport);
        }

        GeckoAppShell.registerGeckoEventListener("Viewport:UpdateAndDraw", this);
        GeckoAppShell.registerGeckoEventListener("Viewport:UpdateLater", this);

        sendResizeEventIfNecessary();
    }

    public void beginDrawing(int width, int height) {
        beginTransaction(mTileLayer);

        if (mBufferSize.width != width || mBufferSize.height != height) {
            mBufferSize = new IntSize(width, height);

            // Reallocate the buffer if necessary

            // * 2 because it's a 16-bit buffer (so 2 bytes per pixel).
            int size = mBufferSize.getArea() * 2;
            if (mBuffer == null || mBuffer.capacity() != size) {
                // Free the old buffer
                if (mBuffer != null) {
                    GeckoAppShell.freeDirectBuffer(mBuffer);
                    mBuffer = null;
                }

                mBuffer = GeckoAppShell.allocateDirectBuffer(size);
            }
        }
    }

    private void updateViewport(String viewportDescription, final boolean onlyUpdatePageSize) {
        try {
            JSONObject viewportObject = new JSONObject(viewportDescription);

            // save and restore the viewport size stored in java; never let the
            // JS-side viewport dimensions override the java-side ones because
            // java is the One True Source of this information, and allowing JS
            // to override can lead to race conditions where this data gets clobbered.
            FloatSize viewportSize = getLayerController().getViewportSize();
            mGeckoViewport = new ViewportMetrics(viewportObject);
            mGeckoViewport.setSize(viewportSize);

            LayerController controller = getLayerController();
            PointF displayportOrigin = mGeckoViewport.getDisplayportOrigin();
            mTileLayer.setOrigin(PointUtils.round(displayportOrigin));
            mTileLayer.setResolution(mGeckoViewport.getZoomFactor());

            if (onlyUpdatePageSize) {
                // Don't adjust page size when zooming unless zoom levels are
                // approximately equal.
                if (FloatUtils.fuzzyEquals(controller.getZoomFactor(),
                        mGeckoViewport.getZoomFactor()))
                    controller.setPageSize(mGeckoViewport.getPageSize());
            } else {
                Log.d(LOGTAG, "Received viewport update from gecko");
                controller.setViewportMetrics(mGeckoViewport);
                controller.abortPanZoomAnimation();
            }
        } catch (JSONException e) {
            Log.e(LOGTAG, "Bad viewport description: " + viewportDescription);
            throw new RuntimeException(e);
        }
    }

    /*
     * TODO: Would be cleaner if this took an android.graphics.Rect instead, but that would require
     * a little more JNI magic.
     */
    public void endDrawing(int x, int y, int width, int height, String metadata) {
        synchronized (getLayerController()) {
            try {
                updateViewport(metadata, !mUpdateViewportOnEndDraw);
                mUpdateViewportOnEndDraw = false;
                Rect rect = new Rect(x, y, x + width, y + height);

                if (mTileLayer instanceof SingleTileLayer)
                    ((SingleTileLayer)mTileLayer).invalidate(rect);
            } finally {
                endTransaction(mTileLayer);
            }
        }
    }

    public ViewportMetrics getGeckoViewportMetrics() {
        // Return a copy, as we modify this inside the Gecko thread
        if (mGeckoViewport != null)
            return new ViewportMetrics(mGeckoViewport);
        return null;
    }

    public Bitmap getBitmap() {
        // Begin a tile transaction, otherwise the buffer can be destroyed while
        // we're reading from it.
        beginTransaction(mTileLayer);
        try {
            if (mBuffer == null || mBufferSize.width <= 0 || mBufferSize.height <= 0)
                return null;
            try {
                Bitmap b = Bitmap.createBitmap(mBufferSize.width, mBufferSize.height,
                                               CairoUtils.cairoFormatTobitmapConfig(mFormat));
                b.copyPixelsFromBuffer(mBuffer.asIntBuffer());
                return b;
            } catch (OutOfMemoryError oom) {
                Log.w(LOGTAG, "Unable to create bitmap", oom);
                return null;
            }
        } finally {
            endTransaction(mTileLayer);
        }
    }

    /** Returns the back buffer. This function is for Gecko to use. */
    public ByteBuffer lockBuffer() {
        return mBuffer;
    }

    /**
     * Gecko calls this function to signal that it is done with the back buffer. After this call,
     * it is forbidden for Gecko to touch the buffer.
     */
    public void unlockBuffer() {
        /* no-op */
    }

    @Override
    public void geometryChanged() {
        /* Let Gecko know if the screensize has changed */
        sendResizeEventIfNecessary();
        render();
    }

    /* Informs Gecko that the screen size has changed. */
    private void sendResizeEventIfNecessary() {
        DisplayMetrics metrics = new DisplayMetrics();
        GeckoApp.mAppContext.getWindowManager().getDefaultDisplay().getMetrics(metrics);

        if (metrics.widthPixels == mScreenSize.width &&
                metrics.heightPixels == mScreenSize.height) {
            return;
        }

        mScreenSize = new IntSize(metrics.widthPixels, metrics.heightPixels);
        int maxSize = getLayerController().getView().getMaxTextureSize();

        // XXX Introduce tiling to solve this?
        if (mScreenSize.width > maxSize || mScreenSize.height > maxSize)
            throw new RuntimeException("Screen size of " + mScreenSize + " larger than maximum texture size of " + maxSize);

        // Round to next power of two until we use NPOT texture support
        IntSize bufferSize = new IntSize(Math.min(maxSize, IntSize.nextPowerOfTwo(mScreenSize.width + LayerController.MIN_BUFFER.width)),
                                         Math.min(maxSize, IntSize.nextPowerOfTwo(mScreenSize.height + LayerController.MIN_BUFFER.height)));

        Log.i(LOGTAG, "Screen-size changed to " + mScreenSize);
        GeckoEvent event = new GeckoEvent(GeckoEvent.SIZE_CHANGED,
                                          bufferSize.width, bufferSize.height,
                                          metrics.widthPixels, metrics.heightPixels);
        GeckoAppShell.sendEventToGecko(event);
    }

    @Override
    public void viewportSizeChanged() {
        mViewportSizeChanged = true;
    }

    @Override
    public void render() {
        adjustViewportWithThrottling();
    }

    private void adjustViewportWithThrottling() {
        if (!getLayerController().getRedrawHint())
            return;

        if (mPendingViewportAdjust)
            return;

        long timeDelta = System.currentTimeMillis() - mLastViewportChangeTime;
        if (timeDelta < MIN_VIEWPORT_CHANGE_DELAY) {
            getLayerController().getView().postDelayed(
                new Runnable() {
                    public void run() {
                        mPendingViewportAdjust = false;
                        adjustViewport();
                    }
                }, MIN_VIEWPORT_CHANGE_DELAY - timeDelta);
            mPendingViewportAdjust = true;
            return;
        }

        adjustViewport();
    }

    private void adjustViewport() {
        ViewportMetrics viewportMetrics =
            new ViewportMetrics(getLayerController().getViewportMetrics());

        PointF viewportOffset = viewportMetrics.getOptimumViewportOffset(mBufferSize);
        viewportMetrics.setViewportOffset(viewportOffset);
        viewportMetrics.setViewport(viewportMetrics.getClampedViewport());

        GeckoAppShell.sendEventToGecko(new GeckoEvent(viewportMetrics));
        if (mViewportSizeChanged) {
            mViewportSizeChanged = false;
            GeckoAppShell.viewSizeChanged();
        }

        mLastViewportChangeTime = System.currentTimeMillis();
    }

    public void handleMessage(String event, JSONObject message) {
        if ("Viewport:UpdateAndDraw".equals(event)) {
            mUpdateViewportOnEndDraw = true;

            // Redraw everything.
            Rect rect = new Rect(0, 0, mBufferSize.width, mBufferSize.height);
            GeckoAppShell.sendEventToGecko(new GeckoEvent(GeckoEvent.DRAW, rect));
        } else if ("Viewport:UpdateLater".equals(event)) {
            mUpdateViewportOnEndDraw = true;
        }
    }
}

