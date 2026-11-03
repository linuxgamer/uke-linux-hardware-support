# Each fragment may exit early; isolate it so the next service still migrates.
/^# @(bluetooth|touch)-migration-(save|restore)$/ {
    split($2, fields, "-")
    print "("
    if (fields[1] == "@touch")
        print "old=uke-touchscreen; new=uke-boot-integration-touch; unit=uke-touchscreen.service; phase=" fields[3]
    else
        print "old=uke-bluetooth; new=uke-boot-integration; unit=uke-bluetooth-address.service; phase=" fields[3]
    while ((getline line < fragment) > 0) {
        gsub(/%/, "%%", line)
        print line
    }
    close(fragment)
    print ") || exit 1"
    next
}
{ print }
