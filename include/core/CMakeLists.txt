set (_headers
    action.h
    atoms.h
    core.h
    countedlist.h
    global.h
    icon.h
    match.h
    modifierhandler.h
    option.h
    output.h
    plugin.h
    point.h
    propertywriter.h
    privateunion.h
    rect.h
    region.h
    screen.h
    serialization.h
    session.h
    size.h
    string.h
    window.h
    wrapsystem.h
    abiversion.h
    deg2rad.h
)

install (
    FILES ${_headers}
    DESTINATION ${COMPIZ_DESTDIR}${includedir}/compiz/core
)
