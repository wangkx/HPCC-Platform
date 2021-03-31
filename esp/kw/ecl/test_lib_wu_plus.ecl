import lib_WorkunitServices;
 
Layout_Person := RECORD
  UNSIGNED1 PersonID;
  STRING15  FirstName;
  STRING25  LastName;
END;

allPeople := DATASET([ {1,'Fred','Smith'},
                       {2,'Joe','Blow'},
                       {3,'Jane','Smith'}],Layout_Person);

somePeople := allPeople(LastName = 'Smith');

dTestWorkunits				:=	lib_WorkunitServices.WorkunitServices.WorkUnitList('','','','','addthis');
count1 := count(dTestWorkunits);

dTestWorkunitStats				:=	lib_WorkunitServices.WorkunitServices.WorkunitStatistics('W20160119-113728');
		
//  Outputs  ---
count1;
somePeople;
dTestWorkunits;
dTestWorkunitStats
