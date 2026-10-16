# OrangeFox recovery packaging

Updated 14 October 2026. Fedora Rawhide AArch64 package:
`uke-orangefox-recovery`.

The package delivers the existing OrangeFox v003 preview for Xiaomi Pad 7 /
POCO Pad X1 as checksummed image data, with source and license records. RPM
installation and updates do not flash partitions or change the active slot.
The withdrawn Alpha 1 release is no longer used.

The IMG requires the installed Android boot stack and is not a temporary
`fastboot boot` image. ZIP installation is unavailable. Package checks and
recovery hardware validation are separate; read the installed compatibility
and rollback records before a device trial.

`make validate` checks the source contract, `make srpm` prepares the data
package, and `make audit` inspects the locked image ramdisk and ZIP on the host.
`make track-stable` admits eligible stable releases only after their complete
artifact/source checks pass. Preview updates require an explicit reviewed pin.
The package delivers the pinned recovery images as data. Python is no longer
an admission blocker; the image still needs its normal payload and device checks.

[Delivery](docs/DELIVERY.md) · [Source offer](docs/SOURCE-OFFER.md) ·
[Former automation](docs/AUTOMATION.md) ·
[Recovery source](https://github.com/MCC45TR/orangefox_device_xiaomi_uke) ·
[COPR](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-test/)
