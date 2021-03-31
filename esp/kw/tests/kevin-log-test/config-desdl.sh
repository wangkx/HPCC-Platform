#!/bin/bash

/home/lexis/runtime/opt/HPCCSystems/bin/esdl publish wsanalytics.xml
/home/lexis/runtime/opt/HPCCSystems/bin/esdl bind-service fsma_esp 8064 wsanalytics.1 WsAnalytics --overwrite --config ws_analytics-fsma-binding-test-small-2.xml
