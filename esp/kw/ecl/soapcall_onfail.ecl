rWURequest  := RECORD
        string                    Wuid{XPATH('Wuid'),maxlength(20)}    :=  'W20160411-141717';
    end;

    rWUResponse := RECORD
        STRING                    Wuid{XPATH('Workunit/Wuid'),maxlength(20)};
    END;

    rWUResponse genResult() := TRANSFORM
        SELF.Wuid := 'unknown';
    END;
/*
outRecord := RECORD
  STRING name{xpath('Name')};
  UNSIGNED6 id{XPATH('ADL')};
  //REAL8 score;
END;

outRecord genDefault1() := TRANSFORM
  SELF.name := FAILMESSAGE;
  SELF.id := FAILCODE;
  //SELF.score := (REAL8)FAILMESSAGE('ip');
END;*/
 
outRecord := RECORD
  STRING Wuid;
END;

outRecord genDefault1() := TRANSFORM
  SELF.Wuid := FAILMESSAGE;
END;
 
    res :=    soapcall('http://10.176.152.188:8010/WsWorkunits',
                       'WUInfo',
                        rWURequest,
                        dataset(rWUResponse),
												LITERAL, 
                        XPATH('WUInfoResponse'),
                   //   ONFAIL(genResult()),
                        TIMEOUT(3000),
												onfail(genDefault1()),
												httpheader('Content-Encoding', 'gzip'),
												httpheader('MyHeader', 'gzip')
                        );
   RETURN res[1].Wuid;
