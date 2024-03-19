build_example(
  NAME lab3
  SOURCE_FILES lab-03-04-capacity/lab3.cc
  LIBRARIES_TO_LINK
    ${libwifi}
    ${libapplications}
    ${libolsr}
    ${libnetanim}
	${libflow-monitor}
	${libcsma}
    ${libbridge}
)

build_example(
  NAME lab5
  SOURCE_FILES lab-05-06-mcs/lab5.cc
  LIBRARIES_TO_LINK
    ${libwifi}
    ${libapplications}
    ${libolsr}
    ${libnetanim}
	${libflow-monitor}
	${libcsma}
    ${libbridge}
)

build_example(
  NAME lab7
  SOURCE_FILES lab-07-08-dcf/lab7-8-cw.cc
  LIBRARIES_TO_LINK
    ${libwifi}
    ${libapplications}
    ${libolsr}
    ${libnetanim}
	${libflow-monitor}
	${libcsma}
    ${libbridge}
)

build_example(
  NAME lab9
  SOURCE_FILES lab-09-jain/lab9.cc
  LIBRARIES_TO_LINK
    ${libwifi}
    ${libapplications}
    ${libolsr}
    ${libnetanim}
	${libflow-monitor}
	${libcsma}
    ${libbridge}
)

build_example(
  NAME model-colocviu
  SOURCE_FILES model-colocviu/model-colocviu.cc
  LIBRARIES_TO_LINK
    ${libwifi}
    ${libapplications}
    ${libolsr}
    ${libnetanim}
	${libflow-monitor}
	${libcsma}
    ${libbridge}
)

build_example(
  NAME lab9-wifi-spatial-reuse
  SOURCE_FILES lab9-wifi-spatial-reuse.cc
  LIBRARIES_TO_LINK
    ${libwifi}
    ${libapplications}
    ${libolsr}
    ${libnetanim}
	${libflow-monitor}
	${libcsma}
    ${libbridge}
)


