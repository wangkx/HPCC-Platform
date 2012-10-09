require([
	"dojo/_base/declare", "dojo/_base/xhr",	"dojo/dom", 
	"dijit/layout/_LayoutWidget", "dijit/_TemplatedMixin",	"dijit/_WidgetsInTemplateMixin", 
	"dijit/layout/BorderContainer",
	"dijit/layout/TabContainer",
	"dijit/layout/ContentPane",
	"dijit/registry", 
	
	"dojo/_base/connect", 
	"dojo/aspect", "dojo/json", "dojo/query", "dojo/store/Memory", "dojo/store/Observable",
	"dijit/Tree", "dijit/tree/ObjectStoreModel", "dijit/tree/dndSource",
    "dijit/Menu", "dijit/MenuItem", "dijit/CheckedMenuItem", "dijit/MenuSeparator", "dijit/PopupMenuItem", 
	"genesis/scripts/ESPBase", "genesis/scripts/ViewSettingsWidget",
	"dojo/text!./templates/ViewSettingsWidget.html"
], function (declare, xhr, dom,	_LayoutWidget, _TemplatedMixin, _WidgetsInTemplateMixin,
	BorderContainer, TabContainer, ContentPane, registry, 
	connect, aspect, json, query, Memory, Observable, Tree, ObjectStoreModel, dndSource,	
	Menu, MenuItem, CheckedMenuItem, MenuSeparator, PopupMenuItem, 
	ESPBase, NavTreeWidget,
	template) {
    return declare("ViewSettingsWidget", [_LayoutWidget, _TemplatedMixin, _WidgetsInTemplateMixin], {
        templateString: template,
        baseClass: "ViewSettingsWidget",
        _baseURL: '',
		_data: null,
		centralPane: null,
        
		buildRendering: function (args) {
            this.inherited(arguments);
        },
		
		addTabForSubfolder: function (subfolder) {
			var SubfolderPane =	new dijit.layout.ContentPane({
					title: subfolder.Name
				})
			this.centralPane.addChild(SubfolderPane);
		},
		
		getNodeSettings: function () {
			var base = new ESPBase({ });
            var serverUrl = base.getBaseURL("WsGenesis") + "/GetGSNodeSettings.json";
			var svnPath = dojo.queryToObject(dojo.doc.location.search.substr((dojo.doc.location.search.substr(0, 1) == "?" ? 1 : 0)))["SVNPath"];
			if (svnPath) {
				console.log(svnPath);
				serverUrl += ('?SVNPath=' + svnPath);
			}
			
			var xhrArgs = {
				url: serverUrl,
				handleAs: "json",
				preventCache: true
			}

			var context = this;
			var deferred = dojo.xhrGet(xhrArgs);

			deferred.then(
				function(response){
					//console.log(json.stringify(response));
                    var nodeInfo = response.GetGSNodeSettingsResponse.NodeSettings;
					if (nodeInfo && nodeInfo.SubFolders && nodeInfo.SubFolders.SubFolder)
					{
						var subfolders = nodeInfo.SubFolders.SubFolder;
						if (subfolders.length > 0) {
							for (var i = 0; i < subfolders.length; ++i) {
								context.addTabForSubfolder(subfolders[i]);
							}
						}
					}
				},
				function(error){
					console.log("Failed to read Nav data");
				}
			);
		},
				
        postCreate: function (args) {
            this.inherited(arguments);
			this._initControls();
		},

		startup: function (args) {
            this.inherited(arguments);
        },

        resize: function (args) {
            this.inherited(arguments);
            this.borderContainer.resize();
        },

        layout: function (args) {
            this.inherited(arguments);
        },
		
        //  Implementation  ---
        _initControls: function () {
			this.getNodeSettings();

			this.borderContainer = registry.byId(this.id + "BorderContainer");		
			//this.centralPane = registry.byId(this.id + "CentralPane");
			this.centralPane = new dijit.layout.TabContainer({
				region: "center",
				id: "contentTabs",
				tabPosition: "top",
				"class": "centerPanel"
			})
			this.borderContainer.addChild( this.centralPane );
        },

		resetPage: function () {
			console.log("resetPage");
        },
		
		_onSubmit: function (evt) {
			console.log("onSubmit");
            this.resetPage();
            var context = this;
			//add more
        },
		
        init: function (target) { 
		}
	});
});