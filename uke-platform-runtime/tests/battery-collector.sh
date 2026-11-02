#!/usr/bin/env bash
# Synthetic sysfs only. Does not activate UPower, D-Bus or a hardware provider.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -r -- "$work"' EXIT
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror "$root/tests/provider-identity.cpp" -o "$work/provider-identity"
"$work/provider-identity" "$work/provider-fixture"
make -C "$root" outdir="$work/build" "$work/build/uke-battery"
sys=$work/sys
mkdir -p "$sys/firmware/devicetree/base" "$sys/class/power_supply" "$sys/devices/charger" "$sys/bus/auxiliary/drivers/qcom_battmgr.pmic_glink_power_supply"
printf 'xiaomi,uke\0qcom,sm7675\0' > "$sys/firmware/devicetree/base/compatible"
ln -s "$sys/bus/auxiliary/drivers/qcom_battmgr.pmic_glink_power_supply" "$sys/devices/charger/driver"
for name in bat usb; do
    supply=$sys/class/power_supply/qcom-battmgr-$name
    mkdir "$supply"
    ln -s "$sys/devices/charger" "$supply/device"
done
printf '1 1 1 1 1 0 0 0 0 0 100 99\n' > "$sys/devices/charger/uke_battmgr_state"
printf 'Discharging\n' > "$sys/class/power_supply/qcom-battmgr-bat/status"
printf 'Good\n' > "$sys/class/power_supply/qcom-battmgr-bat/health"
printf 'NVT\n' > "$sys/class/power_supply/qcom-battmgr-bat/manufacturer"
printf 'BMXX\n' > "$sys/class/power_supply/qcom-battmgr-bat/model_name"
printf '%s\n' -500000 > "$sys/class/power_supply/qcom-battmgr-bat/current_now"
printf '%s\n' 4000000 > "$sys/class/power_supply/qcom-battmgr-bat/voltage_now"
printf '%s\n' -2000000 > "$sys/class/power_supply/qcom-battmgr-bat/power_now"
printf '0\n' > "$sys/class/power_supply/qcom-battmgr-bat/cycle_count"
printf '1\n' > "$sys/class/power_supply/qcom-battmgr-usb/online"
mkdir -p "$sys/class/thermal/thermal_zone0"
printf 'pmic_bcl_current\n' > "$sys/class/thermal/thermal_zone0/type"
printf '125000\n' > "$sys/class/thermal/thermal_zone0/temp"
"$work/build/uke-battery" --sysfs-root "$sys" > "$work/snapshot.json"
jq -e '.pack_variant.supplier_hint == "nvt" and (.pack_variant.qualified | not) and (.pack_variant.electrical_profile_applied | not)' "$work/snapshot.json" >/dev/null
for supplier in SUNWODA Unknown; do
    printf '%s\n' "$supplier" > "$sys/class/power_supply/qcom-battmgr-bat/manufacturer"
    "$work/build/uke-battery" --sysfs-root "$sys" > "$work/variant.json"
    expected=unknown
    [[ $supplier != SUNWODA ]] || expected=sunwoda
    jq -e --arg expected "$expected" '.pack_variant.supplier_hint==$expected and (.pack_variant.electrical_profile_applied|not)' "$work/variant.json" >/dev/null
done
printf 'NVT\n' > "$sys/class/power_supply/qcom-battmgr-bat/manufacturer"
jq -e '.schema_version == 1 and .origin == "fixture" and .provider_verified and .snapshot_coherent
    and .external_power_with_negative_battery_current and .fields["battery.power_now"].value == -2000000
    and .fields["battery.cycle_count"].value == 0 and .fields["battery.cycle_count"].validity == "unqualified"
    and .health.capacity_retention_percent == null and (.health.learned_capacity_qualified | not)
    and .health_provenance.reported_cycles.reported == 0 and .health_provenance.reported_cycles.effective_cycles == null
    and (.health_provenance.reported_cycles.semantics_qualified | not)
    and .health_provenance.reported_health.reported == "Good" and (.health_provenance.reported_health.physical_safety_qualified | not)
    and .health_provenance.vendor_soh_percent.reported == null and (.health_provenance.observed_efc_is_lifetime_cycles | not)
    and .observed_efc == null and .forecast.time_to_full_seconds == null and .forecast.reason == "profile_unqualified"
    and .engineering.reported_terminal_power_uw == -2000000 and (.engineering.power_inputs_qualified | not)
    and .engineering.signed_c_rate_per_hour == null and .engineering.nominal_full_energy_uwh == null
    and (.engineering.cell_topology.qualified | not) and .engineering.cell_topology.series_cells == null
    and .engineering.remaining_life_seconds == null and .engineering.remaining_life_reason == "aging_model_unqualified"
    and .efc_coverage.checkpoint == "disabled_unqualified" and .capabilities.read_only and (.capabilities.charge_threshold | not)
    and .power_path.observation == "external_source_online_battery_discharging"
    and .power_path.bypass == null and (.power_path.electrical_isolation_verified | not)' "$work/snapshot.json" >/dev/null
printf '0\n' > "$sys/class/power_supply/qcom-battmgr-bat/current_now"
for reported in Full 'Not charging'; do
    printf '%s\n' "$reported" > "$sys/class/power_supply/qcom-battmgr-bat/status"
    "$work/build/uke-battery" --sysfs-root "$sys" > "$work/hold-reported.json"
    jq -e '.power_path.observation == "external_source_online_hold_reported" and .power_path.charge_hold_reported
        and .power_path.bypass == null and (.power_path.hold_effect_verified | not)
        and .forecast.time_to_target_seconds == null' "$work/hold-reported.json" >/dev/null
done
printf 'Full\n' > "$sys/class/power_supply/qcom-battmgr-bat/status"
printf '100000\n' > "$sys/class/power_supply/qcom-battmgr-bat/current_now"
"$work/build/uke-battery" --sysfs-root "$sys" > "$work/full-positive.json"
jq -e '.power_path.charge_hold_reported and .power_path.observation == "external_source_online_charging_current"
    and .power_path.bypass == null and (.power_path.hold_effect_verified | not)' "$work/full-positive.json" >/dev/null
for reported in Unknown Unexpected; do
    printf '%s\n' "$reported" > "$sys/class/power_supply/qcom-battmgr-bat/status"
    "$work/build/uke-battery" --sysfs-root "$sys" > "$work/unknown-status.json"
    jq -e '.power_path.charge_hold_reported == null and .power_path.bypass == null' "$work/unknown-status.json" >/dev/null
done
printf 'Discharging\n' > "$sys/class/power_supply/qcom-battmgr-bat/status"
printf '%s\n' -500000 > "$sys/class/power_supply/qcom-battmgr-bat/current_now"
jq -e '.fields["battery.temp"].value == null and .safety == "unknown_safety_state"' "$work/snapshot.json" >/dev/null
jq -e 'all(.fields["battery.current_avg"], .fields["battery.power_avg"], .fields["usb.temp"];
    .value == null and .validity == "unsupported")' "$work/snapshot.json" >/dev/null
printf '%s\n' -400000 > "$sys/class/power_supply/qcom-battmgr-bat/current_avg"
printf '%s\n' -1600000 > "$sys/class/power_supply/qcom-battmgr-bat/power_avg"
printf '450\n' > "$sys/class/power_supply/qcom-battmgr-usb/temp"
"$work/build/uke-battery" --sysfs-root "$sys" > "$work/optional-telemetry.json"
jq -e '.fields["battery.current_avg"].value == -400000 and .fields["battery.power_avg"].value == -1600000
    and .fields["usb.temp"].value == 450 and .fields["usb.temp"].validity == "unqualified"
    and .fields["battery.temp"].value == null and .engineering.reported_terminal_power_uw == -2000000
    and .engineering.nominal_full_energy_uwh == null and .forecast.time_to_full_seconds == null
    and .safety == "unknown_safety_state"' "$work/optional-telemetry.json" >/dev/null
printf '%s\n' -2147483649 > "$sys/class/power_supply/qcom-battmgr-bat/current_avg"
printf '%s\n' 2147483648 > "$sys/class/power_supply/qcom-battmgr-bat/power_avg"
printf '2147483647\n' > "$sys/class/power_supply/qcom-battmgr-usb/temp"
"$work/build/uke-battery" --sysfs-root "$sys" > "$work/invalid-optional-telemetry.json"
jq -e 'all(.fields["battery.current_avg"], .fields["battery.power_avg"], .fields["usb.temp"];
    .value == null and .validity == "out_of_range")' "$work/invalid-optional-telemetry.json" >/dev/null
for value in -1 2147483648 missing; do
    if [[ $value == missing ]]; then rm "$sys/class/power_supply/qcom-battmgr-bat/cycle_count";
    else printf '%s\n' "$value" > "$sys/class/power_supply/qcom-battmgr-bat/cycle_count"; fi
    "$work/build/uke-battery" --sysfs-root "$sys" > "$work/cycles-$value.json"
    jq -e '.fields["battery.cycle_count"].value == null and .health_provenance.reported_cycles.reported == null
        and .health_provenance.reported_cycles.effective_cycles == null and .observed_efc == null
        and .health.capacity_retention_percent == null' "$work/cycles-$value.json" >/dev/null
done
printf '12\n' > "$sys/class/power_supply/qcom-battmgr-bat/cycle_count"
printf '95\n' > "$sys/class/power_supply/qcom-battmgr-bat/state_of_health"
"$work/build/uke-battery" --sysfs-root "$sys" > "$work/reported-health.json"
jq -e '.health_provenance.reported_cycles.reported == 12 and .health_provenance.reported_cycles.effective_cycles == null
    and .health_provenance.vendor_soh_percent.reported == 95 and (.health_provenance.vendor_soh_percent.semantics_qualified | not)
    and .health.capacity_retention_percent == null and .observed_efc == null' "$work/reported-health.json" >/dev/null
printf '1 2 1 2 1 1 0 0 -110 0 100 99\n' > "$sys/devices/charger/uke_battmgr_state"
"$work/build/uke-battery" --sysfs-root "$sys" > "$work/stale.json"
jq -e '(.snapshot_coherent | not) and .status == "transport_quiesced" and .recovery == "cold_boot_required" and .fields == {}
    and .physical_charge_stopped == null and .health_provenance == null' "$work/stale.json" >/dev/null
for driver in pmic_glink_power_supply other.pmic_glink_power_supply qcom_battmgr.pmic_glink_power_supply_extra; do
    mkdir -p "$sys/bus/auxiliary/drivers/$driver"
    rm "$sys/devices/charger/driver"
    ln -s "$sys/bus/auxiliary/drivers/$driver" "$sys/devices/charger/driver"
    "$work/build/uke-battery" --sysfs-root "$sys" --inventory > "$work/wrong-driver.json"
    jq -e '(.provider_verified | not) and .status == "provider_unverified" and .fields == null' "$work/wrong-driver.json" >/dev/null
done
rm "$sys/devices/charger/driver"
ln -s "$sys/bus/auxiliary/drivers/qcom_battmgr.pmic_glink_power_supply" "$sys/devices/charger/driver"
"$work/build/uke-battery" --sysfs-root "$sys" --inventory > "$work/qualified-provider.json"
jq -e '.provider_verified and .status == "inventory_only" and .fields == null' "$work/qualified-provider.json" >/dev/null
mkdir -p "$sys/devices/other-charger"
ln -s "$sys/bus/auxiliary/drivers/qcom_battmgr.pmic_glink_power_supply" "$sys/devices/other-charger/driver"
rm "$sys/class/power_supply/qcom-battmgr-usb/device"
ln -s "$sys/devices/other-charger" "$sys/class/power_supply/qcom-battmgr-usb/device"
"$work/build/uke-battery" --sysfs-root "$sys" --inventory > "$work/mismatched-providers.json"
jq -e '(.provider_verified | not) and .status == "provider_unverified" and .fields == null' "$work/mismatched-providers.json" >/dev/null
rm "$sys/class/power_supply/qcom-battmgr-usb/device"
ln -s "$sys/devices/charger" "$sys/class/power_supply/qcom-battmgr-usb/device"
printf 'xiaomi,other\0' > "$sys/firmware/devicetree/base/compatible"
"$work/build/uke-battery" --sysfs-root "$sys" --inventory > "$work/other.json"
jq -e '(.provider_verified | not) and .status == "provider_unverified" and (.fields == null)' "$work/other.json" >/dev/null
if "$work/build/uke-battery" --watch 0 >/dev/null 2>&1; then exit 1; fi
if "$work/build/uke-battery" --interval-seconds 0 >/dev/null 2>&1; then exit 1; fi
if "$work/build/uke-battery" --interval-seconds 61 >/dev/null 2>&1; then exit 1; fi
echo 'Synthetic collector provider, signed power, missing data and stale snapshot checks passed; no Uke device result'
