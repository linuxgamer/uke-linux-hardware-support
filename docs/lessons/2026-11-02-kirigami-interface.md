# UKE-KIRIGAMI-20261102: separate presentation from guarded C++ operations

Date: 2 November 2026.
Environment: isolated Fedora 46 x86_64 container, GCC 16.2.1, Qt 6.12.0,
KDE Frameworks 6.31.0, Kirigami Addons 1.15.0, Breeze/platform integration 6.8.2,
Xcb/Xvfb with the software Qt Quick renderer. No tablet was accessed.

## Decision and boundary

The user explicitly approved a QML presentation layer for the Qt application,
retaining C++ application logic, typed device operations and security. This
supersedes the earlier all-C++ widget presentation requirement for this frontend.
GTK and the shared backend remain unchanged. The prior working widget sources
and complete local patch/archive remain recoverable for comparison.

The replacement uses original Kirigami ApplicationWindow, page-stack navigation,
responsive GlobalDrawer, FormCardPage, FormHeader, FormCard and native form
delegates. It does not imitate those controls with stylesheets. See the official
[FormCard reference](https://api.kde.org/org-kde-kirigamiaddons-formcard-qmlmodule.html)
and [C++ integration guidance](https://develop.kde.org/docs/getting-started/kirigami/advanced-connect_backend/).
Marknote's upstream navigation and the distribution's original System Settings
were design references; their code was not copied or rebuilt as Uke variants.

`model-data` retains the existing bounded fixture parser, control-character
sanitization and catalog labels. The Qt-local C++ Bridge supplies the allowlisted
catalog, typed draft fields, bounded read results and frozen reviews to QML.
It owns each Query on the event loop and cancels it on Stop, navigation or close.
Refresh repeats only the last validated captured read. Raw stdout and stderr stay
distinct; null, zero, negative values, cancellation and unavailable providers are
not converted into success or measurements.

QML cannot submit a raw command or change the backend executable. C++ validates
the action and every parameter before reads or review, retains exact confirmation
tokens and separate consent, revalidates a frozen request before terminal handoff,
and enforces fixture/read-only refusal independently of button state. Existing
authentication and backend approvals remain in the original terminal/backend.
No new privileged method, shell, policy or hardware writer is added.

## Findings and corrections

Real QML loading caught incorrect component properties that a C++ build alone did
not detect. The new native test loads the actual compiled pages and treats QML
load/binding errors as failures. It opens all 126 guides at 480 pixels and checks
their placement in the real window scene. Removed pages are explicitly destroyed;
tests bound retained page count during repeated navigation. The startup deep link
has its own regression after automatic hidden-page removal briefly discarded it.
Native Back cancels the owned read even when navigation retains the draft page.

Qt can warn that a page was temporarily created outside the graphics scene.
Upstream [PageRow::initPage](https://github.com/KDE/kirigami/blob/master/src/controls/PageRow.qml)
creates it under the nonvisual pagesLogic object before inserting it into
ColumnView. The test recognizes only that exact AreaPage/GuidePage diagnostic
from Main.qml, and independently requires each visible guide to enter the correct
window scene. Other QML warnings still fail the test. This exception is not a
blanket suppression of runtime errors.

An accessibility test found an empty name on the choice delegate. Explicit names
now bind to field labels, including numeric/text inputs and review controls.
Backend-origin text uses native FormTextDelegate labels explicitly set to plain
text, preserving control sanitization without interpreting report data as markup.

QML import versions and Kirigami Addons library versions are different namespaces.
Requesting FormCard import 1.1 failed despite installed Addons 1.15. The build now
checks the unversioned QML module and separately requires KF6KirigamiAddons >=1.1
through its actual CMake package. RPM build/runtime requirements match these
modules. New dependencies are Qt Qml/Quick/QuickControls2, ECM, I18n/I18nQml,
Kirigami, Kirigami Addons FormCard and QQC2DesktopStyle. The application selects
`org.kde.desktop` unless explicitly overridden and retains KDE icon/font/palette
preferences. It does not fall back silently to a generic Quick style.

## Validation scope

Four CTest suites pass: retained widget regressions, CLI policy, native appearance,
and the new Kirigami bridge/runtime suite. Tests cover typed values, errors,
cancellation, repeated reads, all guide navigation, native review refusal,
invalidated reviews after draft changes, root refusal, and accessible names.
Fixture mode blocks real reads, writes and handoffs even with exact confirmation.
Normal-user staged CMake installation and rendered UI evidence are recorded under
ignored `uke-control-qt/build-kirigami/`; those checks do not establish tablet,
full Plasma session, authenticated terminal or physical device acceptance.

The staged executable was visually inspected at 1100×760, 720×860 and 480×720,
including the native review sheet and Escape/Back. An isolated Breeze Dark
configuration and 150% scaling were rendered and inspected. Light/dark metadata
reports `Breeze::Style`, `org.kde.desktop`, ten available icons and Noto Sans 10;
the dark window palette is `#202326`, proving that user colors are inherited.
Installed-app logs show no QML load errors, undefined-property errors or binding
loops during these walkthroughs. A real normal-user read-only `charge.status`
query returns `available:false` and an empty threshold list. Its screenshots
are distinct from the explicitly labeled simulation views. No provider or
hardware success is inferred from that unavailable result.

The existing core package's external `uke-kernel-update` transaction dependency
remains a full RPM installation limitation, as documented in the earlier lesson.
No missing provider is fabricated or bypassed. Source publication does not imply
RPM transaction or device acceptance. Screenshots remain local evidence; the
change does not upload them, request COPR builds, update images or operate devices.

## Source publication review

The GUI changes are applied on top of current hardware main while retaining the
unrelated Ubuntu and source-contract updates. The shared client is rebuilt from
that checkout before rerunning the four Qt suites. Existing source-validation
workflows remain unchanged. The Qt and lesson paths are outside the package
families' watched inputs, so the existing COPR request guard selects no family
for this source-only change. Ubuntu native build workflows require explicit
dispatch and are not restarted by this publication.

Pre-publication checks on hardware main `e06a6d6` pass: the refreshed native
client and Kirigami build, all four Qt CTest suites, `make validate`, and
`make -C packaging/families check`. Those source checks preserve the existing
native-build and device-admission boundaries; they are not RPM transaction or
tablet acceptance results.
