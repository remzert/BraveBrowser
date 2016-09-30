# Gives translation ids for messages.
#
# Messages with placeholders should go like that:
# original message = "I'll buy a <ph name="WAVELENGTH">%d<ex>200</ex></ph> nm laser at <ph name="STORE_NAME">%s<ex>the grocery store</ex></ph>."
# message to get id = "I'll buy a WAVELENGTH nm laser at STORE_NAME."
#
# Messages with line breaks should go like that:
# original message = "She gathered
#wood, charcoal, and
#a sledge hammer."
# message to get id = "She gathered\nwood, charcoal, and\na sledge hammer."

import FP

oldString = "old string"
newString = "new string"
fp = FP.FingerPrint(newString)
fp2 = FP.FingerPrint(oldString)
file_ = open('strings.txt', 'w')
file_.write(str(fp2 & 0x7fffffffffffffffL) + ' - ' + oldString + '\r\n')
file_.write(str(fp & 0x7fffffffffffffffL) + ' - ' + newString + '\r\n')
file_.close()
