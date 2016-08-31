/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.appmenu;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.graphics.Typeface;
import android.util.TypedValue;
import android.graphics.Color;
import android.widget.Switch;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;

import org.chromium.base.Log;
import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.init.ShieldsConfig;
import org.chromium.chrome.browser.omaha.UpdateMenuItemHelper;
import org.chromium.chrome.browser.ChromeApplication;
import org.chromium.chrome.browser.widget.TintedImageButton;
import org.chromium.ui.base.LocalizationUtils;
import org.chromium.ui.interpolators.BakedBezierInterpolator;
import org.chromium.chrome.browser.init.ShieldsConfig;

import java.util.List;
import java.lang.NumberFormatException;

/**
 * ListAdapter to customize the view of items in the list.
 */
class BraveShieldsMenuAdapter extends BaseAdapter {
    /**
     * Regular Android menu item that contains a title and an icon if icon is specified.
     */
    private static final int STANDARD_MENU_ITEM = 0;

    /**
     * Menu item that has two buttons, the first one is a title and the second one is an icon.
     * It is different from the regular menu item because it contains two separate buttons.
     */
    private static final int TITLE_BUTTON_MENU_ITEM = 1;

    /**
     * Menu item that has four buttons. Every one of these buttons is displayed as an icon.
     */
    private static final int THREE_BUTTON_MENU_ITEM = 2;

    /**
     * Menu item that has four buttons. Every one of these buttons is displayed as an icon.
     */
    private static final int FOUR_BUTTON_MENU_ITEM = 3;

    /**
     * Menu item for updating Chrome; uses a custom layout.
     */
    private static final int UPDATE_MENU_ITEM = 4;

    /**
     * The number of view types specified above.  If you add a view type you MUST increment this.
     */
    private static final int VIEW_TYPE_COUNT = 5;

    /** MenuItem Animation Constants */
    private static final int ENTER_ITEM_DURATION_MS = 350;
    private static final int ENTER_ITEM_BASE_DELAY_MS = 80;
    private static final int ENTER_ITEM_ADDL_DELAY_MS = 30;
    private static final float ENTER_STANDARD_ITEM_OFFSET_Y_DP = -10.f;
    private static final float ENTER_STANDARD_ITEM_OFFSET_X_DP = 10.f;

    private static final String BRAVE_SHIELDS_GREY = "#858585";
    private static final String BRAVE_SHIELDS_TEXT = "#FFFFFF";

    private final LayoutInflater mInflater;
    private final List<MenuItem> mMenuItems;
    private final float mDpToPx;
    private BraveShieldsMenuObserver mMenuObserver;

    public BraveShieldsMenuAdapter(List<MenuItem> menuItems,
            LayoutInflater inflater,
            BraveShieldsMenuObserver menuObserver) {
        mMenuItems = menuItems;
        mInflater = inflater;
        mDpToPx = inflater.getContext().getResources().getDisplayMetrics().density;
        mMenuObserver = menuObserver;
    }

    public static String addUpdateCounts(String title, int count, String color) {
        int space = title.indexOf(" ");
        if (-1 == space || title.length() - 1 == space) {
            return title;
        }
        try {
            Integer.parseInt(title.substring(0, space));
            title = title.substring(space + 1);
        }
        catch (NumberFormatException e) {
        }

        return String.format("%1$s %2$s", "<font color=" + color + "><b>" + count + "</b></font>",
          title);
    }

    @Override
    public boolean isEnabled(int position) {
        if (0 == position || 1 == position
          || 3 == position || 4 == position) {
            return false;
        }

        return true;
    }

    @Override
    public int getCount() {
        return mMenuItems.size();
    }

    @Override
    public int getViewTypeCount() {
        return VIEW_TYPE_COUNT;
    }

    @Override
    public int getItemViewType(int position) {
        MenuItem item = getItem(position);
        int viewCount = item.hasSubMenu() ? item.getSubMenu().size() : 1;

        if (item.getItemId() == R.id.update_menu_id) {
            return UPDATE_MENU_ITEM;
        } else if (viewCount == 4) {
            return FOUR_BUTTON_MENU_ITEM;
        } else if (viewCount == 3) {
            return THREE_BUTTON_MENU_ITEM;
        } else if (viewCount == 2) {
            return TITLE_BUTTON_MENU_ITEM;
        }
        return STANDARD_MENU_ITEM;
    }

    @Override
    public long getItemId(int position) {
        return getItem(position).getItemId();
    }

    @Override
    public MenuItem getItem(int position) {
        if (position == ListView.INVALID_POSITION) return null;
        assert position >= 0;
        assert position < mMenuItems.size();
        return mMenuItems.get(position);
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        final MenuItem item = getItem(position);
        switch (getItemViewType(position)) {
            case STANDARD_MENU_ITEM: {
                StandardMenuItemViewHolder holder = null;
                if (convertView == null
                        || !(convertView.getTag() instanceof StandardMenuItemViewHolder)) {
                    holder = new StandardMenuItemViewHolder();
                    if (2 != position) {
                        convertView = mInflater.inflate(R.layout.menu_item, parent, false);
                    } else {
                        convertView = mInflater.inflate(R.layout.brave_shields_switcher, parent, false);
                        setupSwitchClick((Switch)convertView.findViewById(R.id.brave_shields_switch));
                    }
                    holder.text = (TextView) convertView.findViewById(R.id.menu_item_text);
                    holder.image = (AppMenuItemIcon) convertView.findViewById(R.id.menu_item_icon);
                    switch (position) {
                        case 0:
                            convertView.setBackgroundColor(Color.parseColor(BRAVE_SHIELDS_GREY));
                            holder.text.setTypeface(null, Typeface.BOLD);
                            holder.text.setTextColor(Color.parseColor(BRAVE_SHIELDS_TEXT));
                            break;
                        case 1:
                            convertView.setBackgroundColor(Color.parseColor(BRAVE_SHIELDS_GREY));
                            holder.text.setTypeface(null, Typeface.BOLD);
                            holder.text.setTextSize(TypedValue.COMPLEX_UNIT_PX, holder.text.getTextSize() * (float)1.2);
                            holder.text.setTextColor(Color.parseColor(BRAVE_SHIELDS_TEXT));
                            break;
                    }
                    if (2 != position) {
                        convertView.setTag(holder);
                    }
                    else {
                        convertView.setBackgroundColor(Color.parseColor(BRAVE_SHIELDS_GREY));
                    }
                    convertView.setTag(R.id.menu_item_enter_anim_id,
                            buildStandardItemEnterAnimator(convertView, position));
                } else {
                    holder = (StandardMenuItemViewHolder) convertView.getTag();
                }

                if (null != holder.text && null != holder.image) {
                    setupStandardMenuItemViewHolder(holder, convertView, item);
                }
                break;
            }
            default:
                assert false : "Unexpected MenuItem type";
        }
        return convertView;
    }

    private void setupSwitchClick(Switch braveShieldsSwitch) {
        if (null == braveShieldsSwitch) {
            return;
        }
        String host = "";
        if (mMenuItems.size() > 1) {
            host = getItem(1).getTitle().toString();
        }
        if (0 != host.length()) {
            ChromeApplication app = (ChromeApplication)ContextUtils.getApplicationContext();
            if (null != app) {
                if (app.getShieldsConfig().isTopShieldsEnabled(host)) {
                    braveShieldsSwitch.setChecked(true);
                } else {
                    braveShieldsSwitch.setChecked(false);
                }
            }
        }
        braveShieldsSwitch.setOnCheckedChangeListener(new OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView,
              boolean isChecked) {
                String host = "";
                if (mMenuItems.size() > 1) {
                    host = getItem(1).getTitle().toString();
                }
                if (0 != host.length()) {
                    ChromeApplication app = (ChromeApplication)ContextUtils.getApplicationContext();
                    if (null != app) {
                        app.getShieldsConfig().setTopHost(host, isChecked);
                        if (null != mMenuObserver) {
                            mMenuObserver.onMenuTopShieldsChanged(isChecked);
                        }
                    }
                }
            }
        });
    }

    private void setupStandardMenuItemViewHolder(StandardMenuItemViewHolder holder,
            View convertView, final MenuItem item) {
        // Set up the icon.
        Drawable icon = item.getIcon();
        holder.image.setImageDrawable(icon);
        holder.image.setVisibility(icon == null ? View.GONE : View.VISIBLE);
        holder.image.setChecked(item.isChecked());
        holder.text.setText(item.getTitle());
        holder.text.setContentDescription(item.getTitleCondensed());

        boolean isEnabled = item.isEnabled();
        // Set the text color (using a color state list).
        holder.text.setEnabled(isEnabled);
        // This will ensure that the item is not highlighted when selected.
        convertView.setEnabled(isEnabled);

        /*convertView.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                mAppMenu.onItemClick(item);
            }
        });*/
    }

    /**
     * This builds an {@link Animator} for the enter animation of a standard menu item.  This means
     * it will animate the alpha from 0 to 1 and translate the view from -10dp to 0dp on the y axis.
     *
     * @param view     The menu item {@link View} to be animated.
     * @param position The position in the menu.  This impacts the start delay of the animation.
     * @return         The {@link Animator}.
     */
    private Animator buildStandardItemEnterAnimator(final View view, int position) {
        final float offsetYPx = ENTER_STANDARD_ITEM_OFFSET_Y_DP * mDpToPx;
        final int startDelay = ENTER_ITEM_BASE_DELAY_MS + ENTER_ITEM_ADDL_DELAY_MS * position;

        AnimatorSet animation = new AnimatorSet();
        animation.playTogether(
                ObjectAnimator.ofFloat(view, View.ALPHA, 0.f, 1.f),
                ObjectAnimator.ofFloat(view, View.TRANSLATION_Y, offsetYPx, 0.f));
        animation.setDuration(ENTER_ITEM_DURATION_MS);
        animation.setStartDelay(startDelay);
        animation.setInterpolator(BakedBezierInterpolator.FADE_IN_CURVE);

        animation.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationStart(Animator animation) {
                view.setAlpha(0.f);
            }
        });
        return animation;
    }

    /**
     * This builds an {@link Animator} for the enter animation of icon row menu items.  This means
     * it will animate the alpha from 0 to 1 and translate the views from 10dp to 0dp on the x axis.
     *
     * @param views        The list if icons in the menu item that should be animated.
     * @return             The {@link Animator}.
     */
    private Animator buildIconItemEnterAnimator(final ImageView[] views) {
        final boolean rtl = LocalizationUtils.isLayoutRtl();
        final float offsetXPx = ENTER_STANDARD_ITEM_OFFSET_X_DP * mDpToPx * (rtl ? -1.f : 1.f);
        final int maxViewsToAnimate = views.length;

        AnimatorSet animation = new AnimatorSet();
        AnimatorSet.Builder builder = null;
        for (int i = 0; i < maxViewsToAnimate; i++) {
            final int startDelay = ENTER_ITEM_ADDL_DELAY_MS * i;

            Animator alpha = ObjectAnimator.ofFloat(views[i], View.ALPHA, 0.f, 1.f);
            Animator translate = ObjectAnimator.ofFloat(views[i], View.TRANSLATION_X, offsetXPx, 0);
            alpha.setStartDelay(startDelay);
            translate.setStartDelay(startDelay);
            alpha.setDuration(ENTER_ITEM_DURATION_MS);
            translate.setDuration(ENTER_ITEM_DURATION_MS);

            if (builder == null) {
                builder = animation.play(alpha);
            } else {
                builder.with(alpha);
            }
            builder.with(translate);
        }
        animation.setStartDelay(ENTER_ITEM_BASE_DELAY_MS);
        animation.setInterpolator(BakedBezierInterpolator.FADE_IN_CURVE);

        animation.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationStart(Animator animation) {
                for (int i = 0; i < maxViewsToAnimate; i++) {
                    views[i].setAlpha(0.f);
                }
            }
        });
        return animation;
    }

    static class StandardMenuItemViewHolder {
        public TextView text;
        public AppMenuItemIcon image;
    }

    static class CustomMenuItemViewHolder extends StandardMenuItemViewHolder {
        public TextView summary;
    }

    static class ThreeButtonMenuItemViewHolder {
        public TintedImageButton[] buttons = new TintedImageButton[3];
    }

    static class FourButtonMenuItemViewHolder {
        public TintedImageButton[] buttons = new TintedImageButton[4];
    }

    static class TitleButtonMenuItemViewHolder {
        public TextView title;
        public TintedImageButton button;
    }
}
