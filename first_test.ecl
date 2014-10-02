IMPORT KEL04 AS KEL;
IMPORT Header;
IMPORT * FROM KEL04.Null;
E_Person := MODULE
  EXPORT Typ := KEL.typ.uid;
  EXPORT InLayout := RECORD
    KEL.typ.nuid UID;
    KEL.typ.nunk First_Name;
    KEL.typ.nunk Last_Name;
    KEL.typ.nunk Middle_Name;
  END;
  SHARED __Mapping := 'UID:DID,First_Name:fname,Last_Name:lname,Middle_Name:mname';
  EXPORT Header_File_Headers_Invalid := Header.File_Headers((KEL.typ.uid)DID = 0);
  SHARED __d0_Prefiltered := Header.File_Headers((KEL.typ.uid)DID <> 0);
  SHARED __d0 := KEL.FromFlat(__d0_Prefiltered,InLayout,__Mapping);
  EXPORT InData := __d0;
  EXPORT __ST42_Layout := RECORD
    KEL.typ.nunk First_Name;
    KEL.typ.int __RecordCount := 0;
  END;
  EXPORT __ST43_Layout := RECORD
    KEL.typ.nunk Last_Name;
    KEL.typ.int __RecordCount := 0;
  END;
  EXPORT __ST44_Layout := RECORD
    KEL.typ.nunk Middle_Name;
    KEL.typ.int __RecordCount := 0;
  END;
  EXPORT Layout := RECORD
    KEL.typ.nuid UID;
    KEL.typ.ndataset(__ST42_Layout) __ST42;
    KEL.typ.ndataset(__ST43_Layout) __ST43;
    KEL.typ.ndataset(__ST44_Layout) __ST44;
    KEL.typ.int __RecordCount := 0;
  END;
  Layout Person__Rollup(InLayout __r, DATASET(InLayout) __recs) := TRANSFORM
    SELF.__ST42 := __CN(PROJECT(TABLE(__recs,{First_Name,KEL.typ.int __RecordCount := COUNT(GROUP)},First_Name),__ST42_Layout));
    SELF.__ST43 := __CN(PROJECT(TABLE(__recs,{Last_Name,KEL.typ.int __RecordCount := COUNT(GROUP)},Last_Name),__ST43_Layout));
    SELF.__ST44 := __CN(PROJECT(TABLE(__recs,{Middle_Name,KEL.typ.int __RecordCount := COUNT(GROUP)},Middle_Name),__ST44_Layout));
    SELF.__RecordCount := COUNT(__recs);
    SELF := __r;
  END;
  EXPORT __Result := ROLLUP(GROUP(InData,UID,ALL),GROUP,Person__Rollup(LEFT, ROWS(LEFT))) : PERSIST('~temp::KEL::MOD::E_Person::Result',EXPIRE(7));
  EXPORT Result := __UNWRAP(__Result);
  EXPORT SanityCheck := DATASET([{COUNT(Header_File_Headers_Invalid)}],{KEL.typ.int Header_File_Headers_Invalid});
END;
Q_FindPeopleInContext(KEL.typ.unk __P_First_Name, KEL.typ.unk __P_Last_Name) := MODULE
  SHARED __EE59 := E_Person.__Result;
  SHARED __EE71 := __EE59.__ST42;
  __JC144(E_Person.__ST42_Layout __EE71) := __T(__OP2(__CN(__P_First_Name),=,__EE71.First_Name));
  SHARED __EE145 := __EE59(EXISTS(__CHILDJOINFILTER(__EE71,__JC144)));
  SHARED __EE130 := __EE145.__ST43;
  __JC149(E_Person.__ST43_Layout __EE130) := __T(__OP2(__CN(__P_Last_Name),=,__EE130.Last_Name));
  SHARED __EE150 := __EE145(EXISTS(__CHILDJOINFILTER(__EE130,__JC149)));
  EXPORT Res0 := __UNWRAP(__EE150);
  SHARED __EE156 := E_Person.__Result;
  SHARED __EE168 := __EE156.__ST42;
  __JC192(E_Person.__ST42_Layout __EE168) := __T(__OP2(__CN(__P_First_Name),=,__EE168.First_Name));
  SHARED __EE193 := __EE156(EXISTS(__CHILDJOINFILTER(__EE168,__JC192)));
  EXPORT Res1 := __UNWRAP(__EE193);
  SHARED __EE199 := E_Person.__Result;
  SHARED __EE211 := __EE199.__ST43;
  __JC235(E_Person.__ST43_Layout __EE211) := __T(__OP2(__CN(__P_Last_Name),=,__EE211.Last_Name));
  SHARED __EE236 := __EE199(EXISTS(__CHILDJOINFILTER(__EE211,__JC235)));
  EXPORT Res2 := __UNWRAP(__EE236);
END;
