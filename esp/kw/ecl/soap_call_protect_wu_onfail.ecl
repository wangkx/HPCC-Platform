rWUActionResult		:= record
	string20		Wuid;
	string20	Action;
	string		Result;
end;

rWUActionRequest	:= record
	SET OF STRING Wuids{xpath('Wuids/Item')} := ['W20160412-112631', 'W20160412-112240'];
	string ActionType{xpath('ActionType'),maxlength(10)}				:=	'Unprotect';
end;

rWUActionResponse	:= record
	DATASET(rWUActionResult) ActionResults;
	string		ErrorMessage;
end;
 
rWUActionResponse genDefault1() := TRANSFORM
  SELF.ErrorMessage := FAILMESSAGE;
	SELF.ActionResults := [];
END;

dWUSubmitResult	:=	soapcall('http://10.176.152.188:8010/WsWorkunits',
														 'WUAction',
														 rWUActionRequest,
														 rWUActionResponse,
														 xpath('WUActionResponse/Exceptions/Exception'),
														 onfail(genDefault1())
														);
OUTPUT(dWUSubmitResult);	

