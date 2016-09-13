//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version:4.0.30319.42000
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

namespace EpochVS {
	
	
	internal partial class ResolvedSdkReference {
		
		/// <summary>Backing field for deserialized rule.<see cref='Microsoft.Build.Framework.XamlTypes.Rule'/>.</summary>
		private static Microsoft.Build.Framework.XamlTypes.Rule deserializedFallbackRule;
		
		/// <summary>The name of the schema to look for at runtime to fulfill property access.</summary>
		internal const string SchemaName = "ResolvedSdkReference";
		
		/// <summary>The ItemType given in the Rule.DataSource property.  May not apply to every Property's individual DataSource.</summary>
		internal const string PrimaryDataSourceItemType = "SDKReference";
		
		/// <summary>The Label given in the Rule.DataSource property.  May not apply to every Property's individual DataSource.</summary>
		internal const string PrimaryDataSourceLabel = "";
		
		/// <summary> (The "AppXLocation" property).</summary>
		internal const string AppXLocationProperty = "AppXLocation";
		
		/// <summary> (The "OriginalItemSpec" property).</summary>
		internal const string OriginalItemSpecProperty = "OriginalItemSpec";
		
		/// <summary> (The "DisplayName" property).</summary>
		internal const string DisplayNameProperty = "DisplayName";
		
		/// <summary> (The "FrameworkIdentity" property).</summary>
		internal const string FrameworkIdentityProperty = "FrameworkIdentity";
		
		/// <summary> (The "CopyPayload" property).</summary>
		internal const string CopyPayloadProperty = "CopyPayload";
		
		/// <summary> (The "ExpandContent" property).</summary>
		internal const string ExpandContentProperty = "ExpandContent";
		
		/// <summary> (The "ExpandReferenceAssemblies" property).</summary>
		internal const string ExpandReferenceAssembliesProperty = "ExpandReferenceAssemblies";
		
		/// <summary> (The "CopyLocalExpandedReferenceAssemblies" property).</summary>
		internal const string CopyLocalExpandedReferenceAssembliesProperty = "CopyLocalExpandedReferenceAssemblies";
		
		/// <summary>Backing field for the <see cref='Microsoft.Build.Framework.XamlTypes.Rule'/> property.</summary>
		private Microsoft.VisualStudio.ProjectSystem.Properties.IRule rule;
		
		/// <summary>Backing field for the file name of the rule property.</summary>
		private string file;
		
		/// <summary>Backing field for the ItemType property.</summary>
		private string itemType;
		
		/// <summary>Backing field for the ItemName property.</summary>
		private string itemName;
		
		/// <summary>Configured Project</summary>
		private Microsoft.VisualStudio.ProjectSystem.ConfiguredProject configuredProject;
		
		/// <summary>The dictionary of named catalogs.</summary>
		private System.Collections.Immutable.IImmutableDictionary<string, Microsoft.VisualStudio.ProjectSystem.Properties.IPropertyPagesCatalog> catalogs;
		
		/// <summary>Backing field for the <see cref='Microsoft.VisualStudio.ProjectSystem.Properties.IRule'/> property.</summary>
		private Microsoft.VisualStudio.ProjectSystem.Properties.IRule fallbackRule;
		
		/// <summary>Thread locking object</summary>
		private object locker = new object();
		
		/// <summary>Initializes a new instance of the ResolvedSdkReference class.</summary>
		internal ResolvedSdkReference(Microsoft.VisualStudio.ProjectSystem.Properties.IRule rule) {
			this.rule = rule;
		}
		
		/// <summary>Initializes a new instance of the ResolvedSdkReference class.</summary>
		internal ResolvedSdkReference(Microsoft.VisualStudio.ProjectSystem.ConfiguredProject configuredProject, System.Collections.Immutable.IImmutableDictionary<string, Microsoft.VisualStudio.ProjectSystem.Properties.IPropertyPagesCatalog> catalogs, string context, string file, string itemType, string itemName) : 
				this(GetRule(System.Collections.Immutable.ImmutableDictionary.GetValueOrDefault(catalogs, context), file, itemType, itemName)) {
			if ((configuredProject == null)) {
				throw new System.ArgumentNullException("configuredProject");
			}
			this.configuredProject = configuredProject;
			this.catalogs = catalogs;
			this.file = file;
			this.itemType = itemType;
			this.itemName = itemName;
		}
		
		/// <summary>Initializes a new instance of the ResolvedSdkReference class.</summary>
		internal ResolvedSdkReference(Microsoft.VisualStudio.ProjectSystem.Properties.IRule rule, Microsoft.VisualStudio.ProjectSystem.ConfiguredProject configuredProject) : 
				this(rule) {
			if ((rule == null)) {
				throw new System.ArgumentNullException("rule");
			}
			if ((configuredProject == null)) {
				throw new System.ArgumentNullException("configuredProject");
			}
			this.configuredProject = configuredProject;
			this.rule = rule;
			this.file = this.rule.File;
			this.itemType = this.rule.ItemType;
			this.itemName = this.rule.ItemName;
		}
		
		/// <summary>Initializes a new instance of the ResolvedSdkReference class.</summary>
		internal ResolvedSdkReference(Microsoft.VisualStudio.ProjectSystem.ConfiguredProject configuredProject, System.Collections.Immutable.IImmutableDictionary<string, Microsoft.VisualStudio.ProjectSystem.Properties.IPropertyPagesCatalog> catalogs, string context, Microsoft.VisualStudio.ProjectSystem.Properties.IProjectPropertiesContext propertyContext) : 
				this(configuredProject, catalogs, context, GetContextFile(propertyContext), propertyContext.ItemType, propertyContext.ItemName) {
		}
		
		/// <summary>Initializes a new instance of the ResolvedSdkReference class that assumes a project context (neither property sheet nor items).</summary>
		internal ResolvedSdkReference(Microsoft.VisualStudio.ProjectSystem.ConfiguredProject configuredProject, System.Collections.Immutable.IImmutableDictionary<string, Microsoft.VisualStudio.ProjectSystem.Properties.IPropertyPagesCatalog> catalogs) : 
				this(configuredProject, catalogs, "Project", null, null, null) {
		}
		
		/// <summary>Gets the IRule used to get and set properties.</summary>
		public Microsoft.VisualStudio.ProjectSystem.Properties.IRule Rule {
			get {
				return this.rule;
			}
		}
		
		/// <summary>AppXLocation</summary>
		internal Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty AppXLocation {
			get {
				Microsoft.VisualStudio.ProjectSystem.Properties.IRule localRule = this.rule;
				if ((localRule == null)) {
					localRule = this.GeneratedFallbackRule;
				}
				if ((localRule == null)) {
					return null;
				}
				Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty property = ((Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty)(localRule.GetProperty(AppXLocationProperty)));
				if (((property == null) 
							&& (this.GeneratedFallbackRule != null))) {
					localRule = this.GeneratedFallbackRule;
					property = ((Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty)(localRule.GetProperty(AppXLocationProperty)));
				}
				return property;
			}
		}
		
		/// <summary>OriginalItemSpec</summary>
		internal Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty OriginalItemSpec {
			get {
				Microsoft.VisualStudio.ProjectSystem.Properties.IRule localRule = this.rule;
				if ((localRule == null)) {
					localRule = this.GeneratedFallbackRule;
				}
				if ((localRule == null)) {
					return null;
				}
				Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty property = ((Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty)(localRule.GetProperty(OriginalItemSpecProperty)));
				if (((property == null) 
							&& (this.GeneratedFallbackRule != null))) {
					localRule = this.GeneratedFallbackRule;
					property = ((Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty)(localRule.GetProperty(OriginalItemSpecProperty)));
				}
				return property;
			}
		}
		
		/// <summary>DisplayName</summary>
		internal Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty DisplayName {
			get {
				Microsoft.VisualStudio.ProjectSystem.Properties.IRule localRule = this.rule;
				if ((localRule == null)) {
					localRule = this.GeneratedFallbackRule;
				}
				if ((localRule == null)) {
					return null;
				}
				Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty property = ((Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty)(localRule.GetProperty(DisplayNameProperty)));
				if (((property == null) 
							&& (this.GeneratedFallbackRule != null))) {
					localRule = this.GeneratedFallbackRule;
					property = ((Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty)(localRule.GetProperty(DisplayNameProperty)));
				}
				return property;
			}
		}
		
		/// <summary>FrameworkIdentity</summary>
		internal Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty FrameworkIdentity {
			get {
				Microsoft.VisualStudio.ProjectSystem.Properties.IRule localRule = this.rule;
				if ((localRule == null)) {
					localRule = this.GeneratedFallbackRule;
				}
				if ((localRule == null)) {
					return null;
				}
				Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty property = ((Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty)(localRule.GetProperty(FrameworkIdentityProperty)));
				if (((property == null) 
							&& (this.GeneratedFallbackRule != null))) {
					localRule = this.GeneratedFallbackRule;
					property = ((Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty)(localRule.GetProperty(FrameworkIdentityProperty)));
				}
				return property;
			}
		}
		
		/// <summary>CopyPayload</summary>
		internal Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty CopyPayload {
			get {
				Microsoft.VisualStudio.ProjectSystem.Properties.IRule localRule = this.rule;
				if ((localRule == null)) {
					localRule = this.GeneratedFallbackRule;
				}
				if ((localRule == null)) {
					return null;
				}
				Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty property = ((Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty)(localRule.GetProperty(CopyPayloadProperty)));
				if (((property == null) 
							&& (this.GeneratedFallbackRule != null))) {
					localRule = this.GeneratedFallbackRule;
					property = ((Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty)(localRule.GetProperty(CopyPayloadProperty)));
				}
				return property;
			}
		}
		
		/// <summary>ExpandContent</summary>
		internal Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty ExpandContent {
			get {
				Microsoft.VisualStudio.ProjectSystem.Properties.IRule localRule = this.rule;
				if ((localRule == null)) {
					localRule = this.GeneratedFallbackRule;
				}
				if ((localRule == null)) {
					return null;
				}
				Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty property = ((Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty)(localRule.GetProperty(ExpandContentProperty)));
				if (((property == null) 
							&& (this.GeneratedFallbackRule != null))) {
					localRule = this.GeneratedFallbackRule;
					property = ((Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty)(localRule.GetProperty(ExpandContentProperty)));
				}
				return property;
			}
		}
		
		/// <summary>ExpandReferenceAssemblies</summary>
		internal Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty ExpandReferenceAssemblies {
			get {
				Microsoft.VisualStudio.ProjectSystem.Properties.IRule localRule = this.rule;
				if ((localRule == null)) {
					localRule = this.GeneratedFallbackRule;
				}
				if ((localRule == null)) {
					return null;
				}
				Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty property = ((Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty)(localRule.GetProperty(ExpandReferenceAssembliesProperty)));
				if (((property == null) 
							&& (this.GeneratedFallbackRule != null))) {
					localRule = this.GeneratedFallbackRule;
					property = ((Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty)(localRule.GetProperty(ExpandReferenceAssembliesProperty)));
				}
				return property;
			}
		}
		
		/// <summary>CopyLocalExpandedReferenceAssemblies</summary>
		internal Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty CopyLocalExpandedReferenceAssemblies {
			get {
				Microsoft.VisualStudio.ProjectSystem.Properties.IRule localRule = this.rule;
				if ((localRule == null)) {
					localRule = this.GeneratedFallbackRule;
				}
				if ((localRule == null)) {
					return null;
				}
				Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty property = ((Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty)(localRule.GetProperty(CopyLocalExpandedReferenceAssembliesProperty)));
				if (((property == null) 
							&& (this.GeneratedFallbackRule != null))) {
					localRule = this.GeneratedFallbackRule;
					property = ((Microsoft.VisualStudio.ProjectSystem.Properties.IEvaluatedProperty)(localRule.GetProperty(CopyLocalExpandedReferenceAssembliesProperty)));
				}
				return property;
			}
		}
		
		/// <summary>Get the fallback rule if the current rule on disk is missing or a property in the rule on disk is missing</summary>
		private Microsoft.VisualStudio.ProjectSystem.Properties.IRule GeneratedFallbackRule {
			get {
				if (((this.fallbackRule == null) 
							&& (this.configuredProject != null))) {
					System.Threading.Monitor.Enter(this.locker);
					try {
						if ((this.fallbackRule == null)) {
							this.InitializeFallbackRule();
						}
					}
					finally {
						System.Threading.Monitor.Exit(this.locker);
					}
				}
				return this.fallbackRule;
			}
		}
		
		private static Microsoft.VisualStudio.ProjectSystem.Properties.IRule GetRule(Microsoft.VisualStudio.ProjectSystem.Properties.IPropertyPagesCatalog catalog, string file, string itemType, string itemName) {
			if ((catalog == null)) {
				return null;
			}
			return catalog.BindToContext(SchemaName, file, itemType, itemName);
		}
		
		private static string GetContextFile(Microsoft.VisualStudio.ProjectSystem.Properties.IProjectPropertiesContext propertiesContext) {
			if ((propertiesContext.IsProjectFile == true)) {
				return null;
			}
			else {
				return propertiesContext.File;
			}
		}
		
		private void InitializeFallbackRule() {
			if ((this.configuredProject == null)) {
				return;
			}
			Microsoft.Build.Framework.XamlTypes.Rule unboundRule = ResolvedSdkReference.deserializedFallbackRule;
			if ((unboundRule == null)) {
				System.IO.Stream xamlStream = null;
				System.Reflection.Assembly thisAssembly = System.Reflection.Assembly.GetExecutingAssembly();
				try {
					xamlStream = thisAssembly.GetManifestResourceStream("XamlRuleToCode:resolvedsdkreference.xaml");
					Microsoft.Build.Framework.XamlTypes.IProjectSchemaNode root = ((Microsoft.Build.Framework.XamlTypes.IProjectSchemaNode)(System.Xaml.XamlServices.Load(xamlStream)));
					System.Collections.Generic.IEnumerator<System.Object> ruleEnumerator = root.GetSchemaObjects(typeof(Microsoft.Build.Framework.XamlTypes.Rule)).GetEnumerator();
					for (
					; ((unboundRule == null) 
								&& ruleEnumerator.MoveNext()); 
					) {
						Microsoft.Build.Framework.XamlTypes.Rule t = ((Microsoft.Build.Framework.XamlTypes.Rule)(ruleEnumerator.Current));
						if (System.StringComparer.OrdinalIgnoreCase.Equals(t.Name, SchemaName)) {
							unboundRule = t;
							unboundRule.Name = "10c166b0-536d-48b7-9dce-348b8fc81da7";
							ResolvedSdkReference.deserializedFallbackRule = unboundRule;
						}
					}
				}
				finally {
					if ((xamlStream != null)) {
						((System.IDisposable)(xamlStream)).Dispose();
					}
				}
			}
			this.configuredProject.Services.AdditionalRuleDefinitions.AddRuleDefinition(unboundRule, "FallbackRuleCodeGenerationContext");
			Microsoft.VisualStudio.ProjectSystem.Properties.IPropertyPagesCatalog catalog = this.configuredProject.Services.PropertyPagesCatalog.GetMemoryOnlyCatalog("FallbackRuleCodeGenerationContext");
			this.fallbackRule = catalog.BindToContext(unboundRule.Name, this.file, this.itemType, this.itemName);
		}
	}
	
	internal partial class ProjectProperties {
		
		private static System.Func<System.Threading.Tasks.Task<System.Collections.Immutable.IImmutableDictionary<string, Microsoft.VisualStudio.ProjectSystem.Properties.IPropertyPagesCatalog>>, object, ResolvedSdkReference> CreateResolvedSdkReferencePropertiesDelegate = new System.Func<System.Threading.Tasks.Task<System.Collections.Immutable.IImmutableDictionary<string, Microsoft.VisualStudio.ProjectSystem.Properties.IPropertyPagesCatalog>>, object, ResolvedSdkReference>(CreateResolvedSdkReferenceProperties);
		
		private static ResolvedSdkReference CreateResolvedSdkReferenceProperties(System.Threading.Tasks.Task<System.Collections.Immutable.IImmutableDictionary<string, Microsoft.VisualStudio.ProjectSystem.Properties.IPropertyPagesCatalog>> namedCatalogs, object state) {
			ProjectProperties that = ((ProjectProperties)(state));
			return new ResolvedSdkReference(that.ConfiguredProject, namedCatalogs.Result, "Project", that.File, that.ItemType, that.ItemName);
		}
		
		/// <summary>Gets the strongly-typed property accessor used to get and set Resolved SDK reference properties.</summary>
		internal System.Threading.Tasks.Task<ResolvedSdkReference> GetResolvedSdkReferencePropertiesAsync() {
			System.Threading.Tasks.Task<System.Collections.Immutable.IImmutableDictionary<string, Microsoft.VisualStudio.ProjectSystem.Properties.IPropertyPagesCatalog>> namedCatalogsTask = this.GetNamedCatalogsAsync();
			return namedCatalogsTask.ContinueWith(CreateResolvedSdkReferencePropertiesDelegate, this, System.Threading.CancellationToken.None, System.Threading.Tasks.TaskContinuationOptions.ExecuteSynchronously, System.Threading.Tasks.TaskScheduler.Default);
		}
	}
}
