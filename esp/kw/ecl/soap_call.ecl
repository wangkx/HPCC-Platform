rWURequest  := RECORD
        string                    Wuid{XPATH('Wuid'),maxlength(20)}    :=  'W20160401-094955';
        INTEGER                   Wait{XPATH('Wait')}                  :=  -1;
        BOOLEAN                   ReturnOnWait{XPATH('ReturnOnWait')}  :=  FALSE;
    end;

    rWUResponse := RECORD
        STRING                    StateID{XPATH('StateID'),maxlength(20)};
    END;

    rWUResponse genResult() := TRANSFORM
        SELF.StateID := '-1';
    END;

    res :=    soapcall('http://10.176.152.188:8010/WsWorkunits',
                       'WUWaitComplete',
                        rWURequest,
                        dataset(rWUResponse),
												LITERAL, 
                        XPATH('WUWaitResponse'),
                   //   ONFAIL(genResult()),
                        TIMEOUT(0),
                        httpheader('Content-Encoding', 'gzip')
                        );
   RETURN res[1].StateID;
