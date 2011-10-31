set (_headers
    action.h
    atoms.h
    core.h
    countedlist.h
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
    window.h
    wrapsystem.h
)

install (
    FILES ${_headers}
    DESTINATION ${COMPIZ_DESTDIR}${includedir}/compiz/core
)
