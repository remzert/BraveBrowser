/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.appmenu;

import android.app.Activity;
import android.content.res.TypedArray;
import android.view.Menu;
import android.view.View;
import android.widget.PopupMenu;
import android.view.ContextThemeWrapper;
import android.graphics.Point;
import android.graphics.Rect;
import android.widget.ListPopupWindow;
import android.widget.PopupWindow;
import android.graphics.drawable.Drawable;
import android.view.MenuItem;
import android.view.LayoutInflater;
import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.animation.AnimatorSet;
import android.os.Build;
import android.view.ViewGroup;
import android.widget.Switch;
import android.view.Surface;
import android.graphics.Typeface;
import android.widget.TextView;
import android.text.Html;

import org.chromium.base.Log;
import org.chromium.base.SysUtils;
import org.chromium.chrome.R;
import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.AnimationFrameTimeHistogram;

import java.util.ArrayList;
import java.util.List;

/**
 * Object responsible for handling the creation, showing, hiding of the BraveShields menu.
 */
public class BraveShieldsMenuHandler {
    private final static String mAdsTrackersCountColor = "#FD5926";
    private final static String mHTTPSUpgradesCountColor = "#119AF8";
    private final static String mScripsBlockedCountColor = "#5C5C5C";
    private final static float LAST_ITEM_SHOW_FRACTION = 0.5f;

    private final Activity mActivity;
    private final int mMenuResourceId;
    private Menu mMenu;
    private ListPopupWindow mPopup;
    private BraveShieldsMenuAdapter mAdapter;
    private AnimatorSet mMenuItemEnterAnimator;
    private AnimatorListener mAnimationHistogramRecorder = AnimationFrameTimeHistogram
            .getAnimatorRecorder("WrenchMenu.OpeningAnimationFrameTimes");
    private BraveShieldsMenuObserver mMenuObserver;
    private final View mHardwareButtonMenuAnchor;

    /**
     * Constructs a BraveShieldsMenuHandler object.
     * @param activity Activity that is using the BraveShieldsMenu.
     * @param menuResourceId Resource Id that should be used as the source for the menu items.
     */
    public BraveShieldsMenuHandler(Activity activity, int menuResourceId) {
        mActivity = activity;
        mMenuResourceId = menuResourceId;
        mAdapter = null;
        mHardwareButtonMenuAnchor = activity.findViewById(R.id.menu_anchor_stub);
    }

    public void addObserver(BraveShieldsMenuObserver menuObserver) {
        mMenuObserver = menuObserver;
    }

    public void show(View anchorView, String host, int adsAndTrackers
            , int httpsUpgrades, int scriptsBlocked) {

        int rotation = mActivity.getWindowManager().getDefaultDisplay().getRotation();
        if (anchorView == null) {
            // This fixes the bug where the bottom of the menu starts at the top of
            // the keyboard, instead of overlapping the keyboard as it should.
            int displayHeight = mActivity.getResources().getDisplayMetrics().heightPixels;
            int widthHeight = mActivity.getResources().getDisplayMetrics().widthPixels;

            // In appcompat 23.2.1, DisplayMetrics are not updated after rotation change. This is a
            // workaround for it. See crbug.com/599048.
            // TODO(ianwen): Remove the rotation check after we roll to 23.3.0.
            if (rotation == Surface.ROTATION_0 || rotation == Surface.ROTATION_180) {
                displayHeight = Math.max(displayHeight, widthHeight);
            } else if (rotation == Surface.ROTATION_90 || rotation == Surface.ROTATION_270) {
                displayHeight = Math.min(displayHeight, widthHeight);
            } else {
                assert false : "Rotation unexpected";
            }

            Rect rect = new Rect();
            mActivity.getWindow().getDecorView().getWindowVisibleDisplayFrame(rect);
            int statusBarHeight = rect.top;
            mHardwareButtonMenuAnchor.setY((displayHeight - statusBarHeight));

            anchorView = mHardwareButtonMenuAnchor;
            //isByPermanentButton = true;
        }

        if (mMenu == null) {
            PopupMenu tempMenu = new PopupMenu(mActivity, anchorView);
            tempMenu.inflate(mMenuResourceId);
            mMenu = tempMenu.getMenu();
        }
        ContextThemeWrapper wrapper = new ContextThemeWrapper(mActivity, R.style.OverflowMenuTheme);
        Point pt = new Point();
        mActivity.getWindowManager().getDefaultDisplay().getSize(pt);
        // Get the height and width of the display.
        Rect appRect = new Rect();
        mActivity.getWindow().getDecorView().getWindowVisibleDisplayFrame(appRect);

        // Use full size of window for abnormal appRect.
        if (appRect.left < 0 && appRect.top < 0) {
            appRect.left = 0;
            appRect.top = 0;
            appRect.right = mActivity.getWindow().getDecorView().getWidth();
            appRect.bottom = mActivity.getWindow().getDecorView().getHeight();
        }

        mPopup = new ListPopupWindow(wrapper, null, android.R.attr.popupMenuStyle);
        mPopup.setModal(true);
        mPopup.setAnchorView(anchorView);
        mPopup.setInputMethodMode(PopupWindow.INPUT_METHOD_NOT_NEEDED);

        Drawable originalBgDrawable = mPopup.getBackground();

        mPopup.setBackgroundDrawable(ApiCompatibilityUtils.getDrawable(
                wrapper.getResources(), R.drawable.edge_menu_bg));
        mPopup.setAnimationStyle(R.style.OverflowMenuAnim);

        // Turn off window animations for low end devices, and on Android M, which has built-in menu
        // animations.
        if (SysUtils.isLowEndDevice() || Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            mPopup.setAnimationStyle(0);
        }

        Rect bgPadding = new Rect();
        mPopup.getBackground().getPadding(bgPadding);

        int popupWidth = wrapper.getResources().getDimensionPixelSize(R.dimen.menu_width)
                + bgPadding.left + bgPadding.right;

        mPopup.setWidth(popupWidth);

        // Extract visible items from the Menu.
        int numItems = mMenu.size();
        List<MenuItem> menuItems = new ArrayList<MenuItem>();
        for (int i = 0; i < numItems; ++i) {
            MenuItem item = mMenu.getItem(i);
            if (1 == i) {
                item.setTitle(host);
            } else if (3 == i) {
                item.setTitle(Html.fromHtml(BraveShieldsMenuAdapter.addUpdateCounts(item.getTitle().toString(), adsAndTrackers, mAdsTrackersCountColor)));
            } else if (4 == i) {
                item.setTitle(Html.fromHtml(BraveShieldsMenuAdapter.addUpdateCounts(item.getTitle().toString(), httpsUpgrades, mHTTPSUpgradesCountColor)));
            } else if (5 == i) {
                item.setTitle(Html.fromHtml(BraveShieldsMenuAdapter.addUpdateCounts(item.getTitle().toString(), scriptsBlocked, mScripsBlockedCountColor)));
            }
            menuItems.add(item);
        }

        Rect sizingPadding = new Rect(bgPadding);
        mAdapter = new BraveShieldsMenuAdapter(menuItems, LayoutInflater.from(wrapper), mMenuObserver);
        mPopup.setAdapter(mAdapter);
        setMenuHeight(menuItems.size(), appRect, pt.y, sizingPadding, 0,
            wrapper);

        mPopup.show();
        mPopup.getListView().setItemsCanFocus(true);

        // Don't animate the menu items for low end devices.
        if (!SysUtils.isLowEndDevice()) {
            mPopup.getListView().addOnLayoutChangeListener(new View.OnLayoutChangeListener() {
                @Override
                public void onLayoutChange(View v, int left, int top, int right, int bottom,
                        int oldLeft, int oldTop, int oldRight, int oldBottom) {
                    mPopup.getListView().removeOnLayoutChangeListener(this);
                    runMenuItemEnterAnimations();
                }
            });
        }
    }

    private void setMenuHeight(int numMenuItems, Rect appDimensions,
            int screenHeight, Rect padding, int footerHeight,
            ContextThemeWrapper wrapper) {
        assert mPopup.getAnchorView() != null;
        View anchorView = mPopup.getAnchorView();
        int[] anchorViewLocation = new int[2];
        anchorView.getLocationInWindow(anchorViewLocation);
        anchorViewLocation[1] -= appDimensions.top;
        int anchorViewImpactHeight = 0;

        // Set appDimensions.height() for abnormal anchorViewLocation.
        if (anchorViewLocation[1] > screenHeight) {
            anchorViewLocation[1] = appDimensions.height();
        }
        int availableScreenSpace = Math.max(anchorViewLocation[1],
                appDimensions.height() - anchorViewLocation[1] - anchorViewImpactHeight);

        availableScreenSpace -= padding.bottom + footerHeight;

        TypedArray a = wrapper.obtainStyledAttributes(new int[]
                {android.R.attr.listPreferredItemHeightSmall, android.R.attr.listDivider});
        int itemRowHeight = a.getDimensionPixelSize(0, 0);
        Drawable itemDivider = a.getDrawable(1);
        int itemDividerHeight = itemDivider != null ? itemDivider.getIntrinsicHeight() : 0;
        a.recycle();

        int numCanFit = availableScreenSpace / (itemRowHeight + itemDividerHeight);

        // Fade out the last item if we cannot fit all items.
        if (numCanFit < numMenuItems) {
            int spaceForFullItems = numCanFit * (itemRowHeight + itemDividerHeight);
            int spaceForPartialItem = (int) (LAST_ITEM_SHOW_FRACTION * itemRowHeight);
            mPopup.setHeight(spaceForFullItems - itemRowHeight + spaceForPartialItem
                    + padding.top + padding.bottom);
        } else {
            int spaceForFullItems = numMenuItems * (itemRowHeight + itemDividerHeight);
            mPopup.setHeight(spaceForFullItems + padding.top + padding.bottom);
        }
    }

    private void runMenuItemEnterAnimations() {
        mMenuItemEnterAnimator = new AnimatorSet();
        AnimatorSet.Builder builder = null;

        ViewGroup list = mPopup.getListView();
        for (int i = 0; i < list.getChildCount(); i++) {
            View view = list.getChildAt(i);
            Object animatorObject = view.getTag(R.id.menu_item_enter_anim_id);
            if (animatorObject != null) {
                if (builder == null) {
                    builder = mMenuItemEnterAnimator.play((Animator) animatorObject);
                } else {
                    builder.with((Animator) animatorObject);
                }
            }
        }

        mMenuItemEnterAnimator.addListener(mAnimationHistogramRecorder);
        mMenuItemEnterAnimator.start();
    }

    public void updateValues(int adsAndTrackers, int httpsUpgrades, int scriptsBlocked) {
        final int fadsAndTrackers = adsAndTrackers;
        final int fhttpsUpgrades = httpsUpgrades;
        final int fscriptsBlocked = scriptsBlocked;
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (!isShowing()) {
                    return;
                }
                ViewGroup list = mPopup.getListView();
                if (null == list || list.getChildCount() < 4) {
                    return;
                }
                // Set Ads and Trackers count
                View menuItemView = list.getChildAt(3);
                if (null == menuItemView) {
                    return;
                }
                TextView menuText = (TextView) menuItemView.findViewById(R.id.menu_item_text);
                menuText.setText(Html.fromHtml(BraveShieldsMenuAdapter.addUpdateCounts(menuText.getText().toString(), fadsAndTrackers, mAdsTrackersCountColor)));
                // Set HTTPS Upgrades count
                menuItemView = list.getChildAt(4);
                if (null == menuItemView) {
                    return;
                }
                menuText = (TextView) menuItemView.findViewById(R.id.menu_item_text);
                menuText.setText(Html.fromHtml(BraveShieldsMenuAdapter.addUpdateCounts(menuText.getText().toString(), fhttpsUpgrades, mHTTPSUpgradesCountColor)));
                // Set Scripts Blocked count
                menuItemView = list.getChildAt(5);
                if (null == menuItemView) {
                    return;
                }
                menuText = (TextView) menuItemView.findViewById(R.id.menu_item_text);
                menuText.setText(Html.fromHtml(BraveShieldsMenuAdapter.addUpdateCounts(menuText.getText().toString(), fscriptsBlocked, mScripsBlockedCountColor)));
            }
        });
    }

    public boolean isShowing() {
        if (null == mPopup) {
            return false;
        }

        return mPopup.isShowing();
    }

    public void hideBraveShieldsMenu() {
        if (isShowing()) {
            mPopup.dismiss();
            mAdapter = null;
        }
    }
}
