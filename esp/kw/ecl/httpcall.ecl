import std;

/******************************************************************************
   Get ECL version from the WUFile service

   @wuid    The workunit id on which to look up the ECL version 
  ******************************************************************************/
  shared getECLVersion(string espURL, string wuid) := function
    resultLayout := record
      string eclVersion {xpath('@eclVersion')};
    end;

    url := espURL + '/WsWorkunits/WUFile?ver_=1.75&amp;Wuid=' + 
           wuid + '&amp;Type=XML&amp;rawxml_=true';

    results := httpcall(url, 'GET', 'text/xml', resultLayout,
                        onfail(transform(resultLayout, self.eclVersion := 'Unknown')));

    return results.eclVersion;
  end;
  
  ecl_version := getECLVersion('http://172.17.193.39:8010/', 'W20210428-134201');
  OUTPUT(ecl_version);