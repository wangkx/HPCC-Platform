var systemTreeData = [ 
	{ id: 1, label: 'Activity', type:'file', root: true, url: '/SMC/Activity', title: 'Display Activity on all target clusters in an environment' },
	{ id: 2, label: 'Clusters', url: '', type:'cluster', root: true, 
		children:[{_reference: 21}, {_reference: 22}] },
	{ id: 21, label: 'Target Clusters', type:'cluster', url: '', 
		children:[{_reference: 210}, {_reference: 211}, {_reference: 212}, {_reference: 213}] },
	{ id: 210, label: 'All Target Servers', type:'cluster', url: '/WsTopology/TpTargetClusterQuery?Type=Root', title:'View details about target clusters and optionally run preflight activities' },
	{ id: 211, label: 'hthor', type:'cluster', url: 'Not Implemented Yet', title:'View details about this target cluster and optionally run preflight activities' },
	{ id: 212, label: 'thor', type:'cluster', url: 'Not Implemented Yet', title:'View details about this target cluster and optionally run preflight activities' },
	{ id: 213, label: 'roxie', type:'cluster', url: 'Not Implemented Yet', title:'View details about this target cluster and optionally run preflight activities' },
	{ id: 22, label: 'Cluster Processes', type:'cluster', url: '',
		children:[{_reference: 220}, {_reference: 221}, {_reference: 222}] },
	{ id: 220, label: 'All Processes', type:'cluster', url: '/WsTopology/TpClusterQuery?Type=ROOT', title:'View details about clusters and optionally run preflight activities' },
	{ id: 221, label: 'mythor', type:'cluster', url: 'Not Implemented Yet' },
	{ id: 222, label: 'myroxie', type:'cluster', url: 'Not Implemented Yet' },
	{ id: 3, label: 'Servers', type:'cluster', url: '', root: true,
		children:[{_reference: 30}, {_reference: 31}, {_reference: 32}] },
	{ id: 30, label: 'All Servers', type:'cluster', url: '/WsTopology/TpServiceQuery?Type=ALLSERVICES', title:'View details about System Support Servers clusters and optionally run preflight activities' },
	{ id: 31, label: 'Dali Server', type:'server', url: 'Not Implemented Yet' },
	{ id: 32, label: 'ESP Servers', type:'cluster', 
		children:[{_reference: 321}, {_reference: 322}, {_reference: 323}] },
	{ id: 321, label: 'ESP Server 1', type:'server', url: 'Not Implemented Yet' },
	{ id: 322, label: 'ESP Server 2', type:'server', url: 'Not Implemented Yet' },
	{ id: 323, label: 'ESP Server 3', type:'server', url: 'Not Implemented Yet' },
	{ id: 4, label: 'Browse Resources', type:'file', root: true, url: '/WsSMC/BrowseResources', title: 'Browse a list of resources available for download, such as the ECL IDE, documentation, examples, etc. These are only available if optional packages are installed on the ESP Server.' },
];

var fileTreeData = [
        { id: 1, label: 'Upload/download File', type:'tool', root: true, url: '/FileSpray/DropZoneFiles', title: 'Upload or download File from a Landing Zone in the environment' },
        { id: 2, label: 'Drop Zones', type:'folder', url: '', title: 'Access folders and files in Drop Zones', root: true,
                children:[{_reference: 21}, {_reference: 22}] },
        { id: 21, label: 'Drop Zone 1', type:'file', url: 'Not Implemented Yet', title: '' },
        { id: 22, label: 'Drop Zone 2', type:'file', url: 'Not Implemented Yet', title: '' },
        { id: 3, label: 'Logical Files', type:'folder', url: '', title: 'Access logical files', root: true,
                children:[{_reference: 31}, {_reference: 32}, {_reference: 33}, {_reference: 34}, {_reference: 35}] },
        { id: 31, label: 'Search', type:'file', url: '/WsDfu/DFUSearch', title: 'Search for Logical Files using a variety of search criteria' },
        { id: 32, label: 'Browse', type:'file', url: '/WsDfu/DFUQuery', title: 'Browse a list of Logical Files' },
        { id: 33, label: 'File View by Scope', type:'file', url: '/WsDfu/DFUFileView', title: 'Browse a list of Logical Files by Scope' },
        { id: 34, label: 'File Relationships', type:'file', url: '/WsSMC/NotInCommunityEdition?form_&EEPortal=http://hpccsystems.com/download', title: 'Search File Relationships' },
        { id: 35, label: 'Space Usage', type:'file', url: '/WsDfu/DFUSpace', title: 'View details about Space Usage' },
        { id: 4, label: 'View Data File', type:'file', root: true, url: 'WsDfu/DFUGetDataColumns?ChooseFile=1', title: 'Allows you to view the contents of a logical file' },
        { id: 5, label: 'DFU Workunits', type:'folder', url: '', title: 'Spray/copy logical files', root: true,
                children:[{_reference: 51}, {_reference: 52},  {_reference: 53},  {_reference: 54}] },
        { id: 51, label: 'Search', type:'file', url: '/FileSpray/DFUWUSearch', title: 'Search for DFU workunits' },
        { id: 52, label: 'Browse', type:'file', url: '/FileSpray/GetDFUWorkunits', title: 'Browse a list of DFU workunits' },
	{ id: 53, label: 'Todays DFU WUs', type:'folder', url: '', 
		children:[] },
	{ id: 54, label: 'This week DFU WUs', type:'folder', url: '' ,
		children:[] },
        { id: 6, label: 'Actions', type:'folder', url: '', title: 'Spray/copy logical files', root: true,
                children:[{_reference: 61}, {_reference: 62}, {_reference: 63}, {_reference: 64}, {_reference: 65}] },
        { id: 61, label: 'Spray Fixed', type:'tool', url: '/FileSpray/SprayFixedInput', title: 'Spray a fixed width file' },
        { id: 62, label: 'Spray CSV', type:'tool', url: '/FileSpray/SprayVariableInput?submethod=csv', title: 'Spray a comma separated value file' },
        { id: 63, label: 'Spray XML', type:'tool', url: '/FileSpray/SprayVariableInput?submethod=xml', title: 'Spray an XML File' },
        { id: 64, label: 'Remote Copy', type:'tool', url: '/FileSpray/CopyInput', title: 'Copy a Logical File from one environment to another' },
        { id: 65, label: 'XRef', type:'tool', url: '/WsDFUXRef/DFUXRefList', title: 'View Xref result details or run the Xref utility' },
];

var treeData531 = { id: 531, label: 'WU 1', type:'file', url: 'Not Implemented Yet' };
var treeData532 = { id: 532, label: 'WU 2', type:'file', url: 'Not Implemented Yet' };

var treeData541 = { id: 541, label: 'WU 1', type:'file', url: 'Not Implemented Yet' };
var treeData542 = { id: 542, label: 'WU 2', type:'file', url: 'Not Implemented Yet' };
var treeData543 = { id: 543, label: 'WU 3', type:'file', url: 'Not Implemented Yet' };

var eclTreeData = [
	{ id: 1, label: 'QuerySets', type:'folder', url: '', title: 'Access query sets', root: true,
		children:[{_reference: 10}, {_reference: 11}, {_reference: 12}] },
	{ id: 10, label: 'All QuerySets', type:'file', url: '/WsWorkunits/WUQuerySets', title: 'Browse Published Queries' },
	{ id: 11, label: 'QuerySet 1', type:'file', url: 'Not Implemented Yet', title: '' },
	{ id: 12, label: 'QuerySet 2', type:'file', url: 'Not Implemented Yet', title: '' },
	{ id: 2, label: 'Ecl Workunits', type:'folder', url: '', title: 'Access Ecl Workunits.', root: true,
		children:[{_reference: 20}, {_reference: 21}, {_reference: 22}, {_reference: 23}] },
	{ id: 20, label: 'Search', type:'file', url: '/WsWorkunits/WUQuery?form_', title: 'Search for ECL workunits' },
	{ id: 21, label: 'Browse', type:'file', url: '/WsWorkunits/WUQuery', title: 'Browse a list of ECL workunits' },
	{ id: 22, label: 'Todays WUs', type:'folder', url: '', 
		children:[] },
	{ id: 23, label: 'This week WUs', type:'folder', url: '' ,
		children:[] },
	{ id: 3, label: 'Run ECL', type:'tool', root: true, url: '/EclDirect/RunEcl', title: 'Execute ECL Script' },
	{ id: 4, label: 'Scheduler', type:'tool', root: true, url: '/WsWorkunits/WuShowScheduled', title: 'Access the ECL Scheduler to view and manage scheduled workunits or events' },
];

var securityTreeData = [
	{ id: 1, label: 'Users', type:'users', root: true, url: '/WsSMC/NotInCommunityEdition?form_&EEPortal=http://hpccsystems.com/download', title: 'Manage Users and permissions' },
	{ id: 2, label: 'Groups', type:'usergroup', root: true, url: '/WsSMC/NotInCommunityEdition?form_&EEPortal=http://hpccsystems.com/download', title: 'Manage User Groups and permissions' },
	{ id: 3, label: 'Permissions', type:'file', root: true, url: '/WsSMC/NotInCommunityEdition?form_&EEPortal=http://hpccsystems.com/download', url: '/WsWorkunits/WUQuery?form_', title: 'Manage permissions' },
];

