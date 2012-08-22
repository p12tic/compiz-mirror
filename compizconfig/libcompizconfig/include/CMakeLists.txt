set (_headers
    ccs-backend.h
    ccs-defs.h
    ccs-list.h
    ccs-object.h
    ccs-string.h
    ccs.h
    )

install (
	FILES ${_headers}
	DESTINATION ${libcompizconfig_includedir}
	)
