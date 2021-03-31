import lib_WorkunitServices;
 
 dTestWorkunits				:=	lib_WorkunitServices.WorkunitServices.WorkUnitList('','','','','addthis');
count1 := count(dTestWorkunits);

dTestWorkunitStats				:=	lib_WorkunitServices.WorkunitServices.WorkunitStatistics('W20160119-113728');
		
//  Outputs  ---
count1;
//dTestWorkunits;
dTestWorkunitStats
