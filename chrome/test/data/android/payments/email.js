/*
 * Copyright 2016 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* global PaymentRequest:false */

/*
 * Launches the PaymentRequest UI that requests email address.
 */
function buy() {  // eslint-disable-line no-unused-vars
  try {
    new PaymentRequest(
        [{supportedMethods: ['visa']}],
        {total: {label: 'Total', amount: {currency: 'USD', value: '5.00'}}},
        {requestPayerEmail: true})
        .show()
        .then(function(resp) {
          resp.complete('success')
              .then(function() {
                print(
                    resp.payerEmail + '<br>' + resp.methodName + '<br>' +
                    JSON.stringify(resp.details, undefined, 2));
              })
              .catch(function(error) {
                print(error.message);
              });
        })
        .catch(function(error) {
          print(error.message);
        });
  } catch (error) {
    print(error.message);
  }
}