// SPDX-License-Identifier: GPL-3.0-or-later
function command(action) {
    const commands = {
        plasma: "/usr/bin/uke-control desktop plasma",
        desktop: "/usr/bin/uke-control desktop status",
        usb: "/usr/bin/uke-control usb status",
        lights: "/usr/bin/uke-control flashlight status",
        flashlightOn: "/usr/bin/pkexec /usr/bin/uke-control flashlight on",
        flashlightOff: "/usr/bin/pkexec /usr/bin/uke-control flashlight off",
        red: "/usr/bin/pkexec /usr/bin/uke-control led set 255 0 0",
        green: "/usr/bin/pkexec /usr/bin/uke-control led set 0 255 0",
        blue: "/usr/bin/pkexec /usr/bin/uke-control led set 0 0 255",
        ledOff: "/usr/bin/pkexec /usr/bin/uke-control led off",
        rotationOn: "/usr/bin/uke-control desktop rotation on",
        rotationOff: "/usr/bin/uke-control desktop rotation off",
        brightnessOn: "/usr/bin/uke-control desktop brightness on",
        brightnessOff: "/usr/bin/uke-control desktop brightness off",
        cdcOn: "/usr/bin/pkexec /usr/bin/uke-control set cdc on --persistent",
        cdcOff: "/usr/bin/pkexec /usr/bin/uke-control set cdc off --persistent",
        host: "/usr/bin/pkexec /usr/bin/uke-control usb request data host",
        device: "/usr/bin/pkexec /usr/bin/uke-control usb request data device"
    }
    return Object.prototype.hasOwnProperty.call(commands, action) ? commands[action] : null
}

function parse(kind, output) {
    const value = JSON.parse(output)
    const schemas = {
        plasma: "org.uke.control.plasma.v1",
        desktop: "org.uke.control.desktop.v1",
        usb: "org.uke.control.usb.v1",
        lights: "org.uke.control.lights.v1"
    }
    if (!value || value.schema !== schemas[kind] || !schemas[kind])
        throw new Error("Unexpected uke-control response")
    if ((kind === "desktop" && !Array.isArray(value.interfaces)) ||
        (kind === "usb" && !Array.isArray(value.typec)) ||
        (kind === "plasma" && [value.rotation, value.automatic_brightness].some(function(item) {
            return item !== null && typeof item !== "boolean"
        })))
        throw new Error("Malformed uke-control state")
    if (kind === "lights") {
        for (const name of ["flashlight", "rgb"]) {
            const light = value[name]
            if (!light || typeof light.available !== "boolean" ||
                (light.available ? typeof light.on !== "boolean" : light.on !== null))
                throw new Error("Malformed light state")
        }
    }
    return value
}

function property(snapshot, service, name) {
    const interfaces = snapshot.interfaces || []
    for (let i = 0; i < interfaces.length; ++i) {
        const item = interfaces[i]
        if (item.service === service && item.available === true && item.properties && item.properties[name])
            return item.properties[name].value
    }
    return null
}
