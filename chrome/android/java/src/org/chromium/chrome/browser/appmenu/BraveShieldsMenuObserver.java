/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.appmenu;

/**
 * Allows monitoring of brave shields menu actions.
 */
public interface BraveShieldsMenuObserver {
    /**
     * Informs when the BraveShields Menu top switch changes.
     * @param isOn Whether top shields are on.
     */
    public void onMenuTopShieldsChanged(boolean isOn);
}
