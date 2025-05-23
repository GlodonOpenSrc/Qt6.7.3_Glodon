// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_PAYMENTS_OFFER_NOTIFICATION_HANDLER_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_PAYMENTS_OFFER_NOTIFICATION_HANDLER_H_

#include "base/containers/flat_set.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "url/gurl.h"

namespace autofill {

class AutofillClient;
class AutofillOfferData;
class AutofillOfferManager;

// The class to handle actions related to the offer notifications. It is owned
// by the AutofillOfferManager and it is one per browser context.
class OfferNotificationHandler {
 public:
  explicit OfferNotificationHandler(AutofillOfferManager* offer_manager);
  OfferNotificationHandler(const OfferNotificationHandler&) = delete;
  OfferNotificationHandler& operator=(const OfferNotificationHandler&) = delete;
  ~OfferNotificationHandler();

  // Dismisses or updates the offer notification.
  void UpdateOfferNotificationVisibility(AutofillClient* client);

  // Clears and set the |shown_notification_ids_| set. Only for tests.
  void ClearShownNotificationIdForTesting();
  void AddShownNotificationIdForTesting(int64_t shown_notification_id);

 private:
  bool ValidOfferExistsForUrl(const GURL& url);
  // Updates the offer notification for a ShoppingService offer if a
  // ShoppingService offer is available.
  void UpdateOfferNotificationForShoppingServiceOffer(
      AutofillClient* client,
      const GURL& url,
      const AutofillOfferData& offer);

  bool ShowShoppingServiceOfferNotificationAutomatically(
      const GURL& url,
      const AutofillOfferData& offer);

  // The reference to the offer manager that owns |this|.
  raw_ptr<AutofillOfferManager> offer_manager_;

  // This set includes the unique id of shown offer notifications in the
  // current browser context. It serves as a cross-tab status tracker for the
  // notification UI.
  base::flat_set<int64_t> shown_notification_ids_;

  base::WeakPtrFactory<OfferNotificationHandler> weak_ptr_factory_{this};
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_PAYMENTS_OFFER_NOTIFICATION_HANDLER_H_
