/*##############################################################################
#   HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
############################################################################## */
define([
    "dojo/_base/declare",
    "dojo/_base/lang",
    "dojo/i18n",
    "dojo/i18n!./nls/hpcc",
    "dojo/dom",
    "dojo/query",
    "dojo/topic",
    "dijit/registry",

    "hpcc/_Widget",
    "hpcc/_TabContainerWidget",
    "hpcc/DelayLoadWidget",
    "hpcc/ECLSourceWidget",
    "hpcc/WsPackageMaps",

    "dojo/text!../templates/PMValidateWidget.html",

    "dijit/layout/BorderContainer",
    "dijit/layout/TabContainer",
    "dijit/layout/ContentPane",
    "dijit/form/Button"
], function (declare, lang, i18n, nlsHPCC, dom, query, topic, registry,
                _Widget, _TabContainerWidget, DelayLoadWidget, EclSourceWidget, WsPackageMaps,
                template) {
    return declare("PMValidateWidget", [_TabContainerWidget], {
        templateString: template,
        baseClass: "PMValidateWidget",
        i18n: nlsHPCC,

        initalized: false,
        targets: null,
        processes: new Array(),

        targetSelectControl: null,
        processSelectControl: null,
        validateButton: null,
        editorControl: null,
        resultControl: null,

        validateActivePackageMapWidget: null,
        validatePackageMapContentWidget: null,
        validatePackageMapContentWidgetLoaded: false,

        buildRendering: function (args) {
            this.inherited(arguments);
        },

        startup: function (args) {
            this.inherited(arguments);
        },

        destroy: function (args) {
            this.inherited(arguments);
        },

        getTitle: function () {
            return this.i18n.ValidateActivePackageMap;
        },

        postCreate: function (args) {
            this.inherited(arguments);
            this.validateActivePackageMapWidget = registry.byId(this.id + "_ValidateActivePackageMap");
            this.validatePackageMapContentWidget = registry.byId(this.id + "_ValidatePackageMapContent");

            this.initControls();
        },

        initControls: function () {
            var context = this;
            this.borderContainer = registry.byId(this.id + "BorderContainer");
            this.targetSelectControl = registry.byId(this.id + "TargetSelect");
            this.processSelectControl = registry.byId(this.id + "ProcessSelect");
            this.validateButton = registry.byId(this.id + "SubmitBtn");
        },

        //  init this page ---
        init: function (params) {
            if (this.initalized)
                return;

            this.initalized = true;
            this.inherited(arguments);
            if (params.params.targets !== undefined)
                this.initSelections(params.params.targets);

            this.editorControl = registry.byId(this.id + "Source");
            this.editorControl.init(params);
            this.initResultDisplay();
        },

        initSelections: function (targets) {
            this.targets = targets;
            if (this.targets.length > 0) {
                var defaultTarget = 0;
                for (var i = 0; i < this.targets.length; ++i) {
                    if ((defaultTarget == 0) && (this.targets[i].Type == 'roxie'))
                        defaultTarget = i; //first roxie
                    this.targetSelectControl.options.push({label: this.targets[i].Name, value: this.targets[i].Name});
                }
                this.targetSelectControl.set("value", this.targets[defaultTarget].Name);
                if (this.targets[defaultTarget].Processes != undefined)
                    this.updateProcessSelections(this.targets[defaultTarget].Name);
            }
        },

        updateProcessSelections: function (targetName) {
            this.processSelectControl.removeOption(this.processSelectControl.getOptions());
            for (var i = 0; i < this.targets.length; ++i) {
                var target = this.targets[i];
                if ((target.Processes != undefined) && ((targetName == '') || (targetName == target.Name)))
                    this.addProcessSelections(target.Processes.Item);
            }
            this.processSelectControl.options.push({label: this.i18n.ANY, value: 'ANY' });
            this.processSelectControl.set("value", '');
        },

        addProcessSelections: function (processes) {
            for (var i = 0; i < processes.length; ++i) {
                var process = processes[i];
                if ((this.processes != null) && (this.processes.indexOf(process) != -1))
                    continue;
                this.processes.push(process);
                this.processSelectControl.options.push({label: process, value: process});
            }
        },

        initResultDisplay: function () {
            this.resultControl = registry.byId(this.id + "_Result");
            this.resultControl.init({sourceMode: 'text/plain', readOnly: true});
            this.resultControl.setText("(Validation result)");
        },

        //  init tab ---
        initTab: function () {
            var currSel = this.getSelectedChild();
            if (currSel.id == this.validatePackageMapContentWidget.id && !this.validatePackageMapContentWidgetLoaded) {
                this.validatePackageMapContentWidgetLoaded = true;
                this.validatePackageMapContentWidget.init({
                    targets: this.targets
                });
            }
        },

        //  action
        _onChangeTarget: function (event) {
            this.processes.length = 0;
            this.targetSelected  = this.targetSelectControl.getValue();
            this.updateProcessSelections(this.targetSelected);
        },

        _onChangeProcess: function (event) {
            this.editorControl.setText('');
            var process = this.processSelectControl.getValue();
            if (process == 'ANY')
                process = '*';

            var context = this;
            WsPackageMaps.getPackage({
                    target: this.targetSelectControl.getValue(),
                    process: process
//                    process: '*'
                }, {
                load: function (name, content) {
                    if (content != '') {
                        context.editorControl.setText(content);;
                    }
                },
                error: function (errMsg, errStack) {
                    context.showErrors(errMsg, errStack);
                }
            });
        },

        _onSubmit: function (evt) {
            var request = { target: this.targetSelectControl.getValue() };
            var content = this.editorControl.getText();
            if (content == '') {
                alert(this.i18n.PackageContentNotSet);
                return;
            }
            request['content'] = content;

            var context = this;
            this.resultControl.setText("");
            this.validateButton.set("disabled", true);
            WsPackageMaps.validatePackage(request, {
                load: function (response) {
                    var responseText = context.validateResponseToText(response);
                    if (responseText == '')
                        context.resultControl.setText(context.i18n.Empty);
                    else {
                        responseText = context.i18n.ValidateResult + responseText;
                        context.resultControl.setText(responseText);
                    }
                    context.validateButton.set("disabled", false);
                },
                error: function (errMsg, errStack) {
                    context.showErrors(errMsg, errStack);
                    context.validateButton.set("disabled", false);
                }
            });
        },

        validateResponseToText: function (response) {
            var text = "";
            if (!lang.exists("Errors", response) || (response.Errors.length < 1))
                text += this.i18n.NoErrorFound;
            else
                text = this.addArrayToText(this.i18n.Errors, response.Errors, text);
            if (!lang.exists("Warnings", response) || (response.Warnings.length < 1))
                text += this.i18n.Warnings;
            else
                text = this.addArrayToText(this.i18n.Warnings, response.Warnings, text);

            text += "\n";
            text = this.addArrayToText(this.i18n.QueriesNoPackage, response.queries.Unmatched, text);
            text = this.addArrayToText(this.i18n.PackagesNoQuery, response.packages.Unmatched, text);
            text = this.addArrayToText(this.i18n.FilesNoPackage, response.files.Unmatched, text);
            return text;
        },

        addArrayToText: function (arrayTitle, arrayItems, text) {
            if ((arrayItems.Item != undefined) && (arrayItems.Item.length > 0)) {
                text += arrayTitle + ":\n";
                for (i=0;i<arrayItems.Item.length;i++)
                    text += "  " + arrayItems.Item[i] + "\n";
                text += "\n";
            }
            return text;
        },

/*        resetPage: function () {
            this.editorControl.clearErrors();
            this.editorControl.clearHighlightLines();
            this.updateInput("State", null, "...");
        },

        updateInput: function (name, oldValue, newValue) {
            var input = query("input[id=" + this.id + name + "]", this.summaryForm)[0];
            if (input) {
                var dijitInput = registry.byId(this.id + name);
                if (dijitInput) {
                    dijitInput.set("value", newValue);
                } else {
                    input.value = newValue;
                }
            }
            if (name === "hasCompleted") {
                this.checkIfComplete();
            }
        },*/

        showErrors: function (errMsg, errStack) {
            topic.publish("hpcc/brToaster", {
                Severity: "Error",
                Source: errMsg,
                Exceptions: [{ Message: errStack }]
            });
        }
    });
});
