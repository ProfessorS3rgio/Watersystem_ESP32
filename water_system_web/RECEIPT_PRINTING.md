# Receipt printing

Payment confirmation, billing reprints, and customer usage reprints share the same DMBC receipt layout, including the existing association seal. The preview is black and white to suit thermal paper. Cash amounts use `PHP` for clear currency rendering. Missing historical cash/change values display a dash.

Printing uses the browser and the installed Windows printer driver. The old `localhost:3000/print` integration has been replaced; no separate Node print service is needed.

## XP-58IIH setup

1. Install the Xprinter Windows driver and select the XP-58IIH in the print dialog.
2. Choose a **58 mm roll** paper form in printer preferences. The receipt content is **48 mm** wide, centered on the roll.
3. Set scale to **100%**, margins to **none**, and disable browser headers and footers. If shown, allow the document's custom paper size. The page length is calculated after the logo loads.
4. Allow pop-ups for this website. The receipt window opens immediately, waits for the seal, then opens the print dialog. Its Print receipt button allows another attempt if the dialog is canceled.
5. Print one receipt to check paper alignment and logo darkness on the actual unit. Driver paper forms can override the browser's page size; adjust the driver form if it adds excess blank paper.

The screen preview uses the same receipt markup and styles as the print window. A browser dialog confirms only that printing was requested; the website cannot confirm physical output or detect an empty roll.

Reprints use the latest payment transaction returned by `/bills/{id}/payment`, ordered by payment date and transaction ID. A receipt shows that individual payment, rather than combining partial payments. Bill balance is explicitly labeled as the current balance for that bill, with an as-of date; it excludes other bills on the account.

## Receipt details

The website now loads the linked bill and reading for each payment: customer class/address/barangay, billing period, due date, previous/present readings, usage, recorded rate, water charge, penalty, bill total, payment method, and collector. The billing period follows `receipt_printer.h`'s month-start through bill-date convention. Historical readings and prices come from the bill record, rather than being recalculated with current customer settings. Missing details are omitted or shown as a dash.

Contact details are configured in `config/receipt.php`, with `RECEIPT_*` overrides listed in `.env.example`. Defaults use an example phone/email and hours, explicitly marked **SAMPLE CONTACT DETAILS** on paper. Replace them with real contact details and set `RECEIPT_CONTACT_IS_EXAMPLE=false` when ready. If Laravel config is cached, clear or rebuild that cache after editing environment values.

## Excess blank paper

Browser **headers and footers must be OFF**. They caused the reported leading blank feed. Also use no margins, 100% scale, and the 58 mm roll form. The receipt itself has only 3 mm of top padding. Browser page controls and the installed driver can add space beyond the receipt CSS.
