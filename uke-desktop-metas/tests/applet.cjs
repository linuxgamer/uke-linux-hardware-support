// Host-only assertions; no Node.js dependency is shipped to the tablet.
const assert = require("node:assert/strict");
const fs = require("node:fs");
const vm = require("node:vm");
const context = vm.createContext({});
vm.runInContext(fs.readFileSync(`${__dirname}/../src/plasma-applet/contents/ui/Control.js`, "utf8"), context);
for (const action of ["plasma", "desktop", "usb", "rotationOn", "rotationOff", "brightnessOn", "brightnessOff", "cdcOn", "cdcOff", "host", "device"])
    assert.match(context.command(action), /^(\/usr\/bin\/pkexec )?\/usr\/bin\/uke-control /);
for (const action of ["__proto__", "constructor", "usb; touch /tmp/unsafe", ""])
    assert.equal(context.command(action), null);
for (const action of ["lights", "flashlightOn", "flashlightOff", "red", "green", "blue", "ledOff"])
    assert.match(context.command(action), /^(\/usr\/bin\/pkexec )?\/usr\/bin\/uke-control /);
assert.equal(context.command("red"), "/usr/bin/pkexec /usr/bin/uke-control led set 255 0 0");
assert.equal(context.parse("lights", '{"schema":"org.uke.control.lights.v1","flashlight":{"available":false,"on":null},"rgb":{"available":false,"on":null}}').rgb.available, false);
for (const output of ['{"schema":"org.uke.control.lights.v1"}', '{"schema":"org.uke.control.lights.v1","flashlight":{"available":"true","on":false},"rgb":{"available":false,"on":null}}'])
    assert.throws(() => context.parse("lights", output));
assert.equal(context.parse("plasma", '{"schema":"org.uke.control.plasma.v1","rotation":null,"automatic_brightness":false}').rotation, null);
for (const output of ["null", "invalid", '{"schema":"wrong"}', '{"schema":"org.uke.control.plasma.v1","rotation":"true"}'])
    assert.throws(() => context.parse("plasma", output));
assert.equal(context.property({}, "UPower", "LidIsClosed"), null);
assert.equal(context.property({interfaces: [{service:"UPower", available:false, properties:{LidIsClosed:{value:true}}}]}, "UPower", "LidIsClosed"), null);
console.log("Applet fixed-command boundary, response schemas and unavailable state passed");
