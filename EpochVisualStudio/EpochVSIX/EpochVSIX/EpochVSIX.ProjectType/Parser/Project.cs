﻿using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class Project
    {
        private Dictionary<string, SourceFile> Files;

        private LexicalScope GlobalScope;


        private Dictionary<string, FunctionSignature> FunctionSignatures;
        private Dictionary<string, Structure> StructureDefinitions;
        private Dictionary<string, SumType> SumTypes;
        private Dictionary<string, StrongAlias> StrongAliases;
        private Dictionary<string, WeakAlias> WeakAliases;

        private IVsHierarchy Hierarchy;
        private DateTime LastParseTime;


        private const uint VSITEMID_ROOT = 0xFFFFFFFE;          // TODO - obtain this in a better way
        private const uint VSITEMID_NIL = 0xFFFFFFFF;


        public Project(IVsHierarchy hierarchy)
        {
            ResetContents();

            LastParseTime = DateTime.MinValue;
            Hierarchy = hierarchy;
        }


        public void ParseIfOutdated()
        {
            if (DateTime.Now < LastParseTime.AddSeconds(15.0))
                return;

            LastParseTime = DateTime.Now;
            ResetContents();
            ParseHierarchy(Hierarchy, VSITEMID_ROOT);
        }


        public void RegisterFunction(FunctionSignature function)
        {
            if (FunctionSignatures.ContainsKey(function.Name.Text))
                FunctionSignatures[function.Name.Text].Overloads.AddRange(function.Overloads);
            else
                FunctionSignatures.Add(function.Name.Text, function);
        }

        public void RegisterGlobalVariable(Variable variable)
        {
            GlobalScope.Variables.Add(variable);
        }

        public void RegisterSourceFile(string path, SourceFile file)
        {
            if (Files.ContainsKey(path))
                Files[path] = file;
            else
                Files.Add(path, file);
        }

        public void RegisterStrongAlias(Token nametoken, StrongAlias alias)
        {
            if (!StrongAliases.ContainsKey(nametoken.Text))
                StrongAliases.Add(nametoken.Text, alias);
        }

        public void RegisterStructureType(Token nametoken, Structure structure)
        {
            if (!StructureDefinitions.ContainsKey(nametoken.Text))
                StructureDefinitions.Add(nametoken.Text, structure);
        }

        public void RegisterSumType(Token nametoken, SumType sumtype)
        {
            if (!SumTypes.ContainsKey(nametoken.Text))
                SumTypes.Add(nametoken.Text, sumtype);
        }

        public void RegisterWeakAlias(Token nametoken, WeakAlias alias)
        {
            if (!WeakAliases.ContainsKey(nametoken.Text))
                WeakAliases.Add(nametoken.Text, alias);
        }


        public List<FunctionSignature> GetAvailableFunctionSignatures()
        {
            return FunctionSignatures.Select(kvp => kvp.Value).ToList();
        }

        public Dictionary<string, Structure> GetAvailableStructureDefinitions()
        {
            return StructureDefinitions;
        }

        public List<string> GetAvailableTypeNames()
        {
            var t = SumTypes.Select(kvp => kvp.Key).Union(
                    StrongAliases.Select(kvp => kvp.Key).Union(
                    WeakAliases.Select(kvp => kvp.Key)
                    ));

            return t.ToList();
        }

        public List<Variable> GetAvailableVariables(string file, int line, int column)
        {
            var ret = new List<Variable>();
            if (GlobalScope != null && GlobalScope.Variables.Count > 0)
                ret.AddRange(GlobalScope.Variables);


            return ret;
        }


        public Structure GetStructureDefinition(string name)
        {
            if (!StructureDefinitions.ContainsKey(name))
                return null;

            return StructureDefinitions[name];
        }


        public bool IsRecognizedFunction(string name)
        {
            return FunctionSignatures.ContainsKey(name);
        }

        public bool IsRecognizedStructureType(string name)
        {
            return StructureDefinitions.ContainsKey(name);
        }

        public bool IsRecognizedType(string name)
        {
            return SumTypes.ContainsKey(name) || StrongAliases.ContainsKey(name) || WeakAliases.ContainsKey(name);
        }


        private void ParseHierarchy(IVsHierarchy hierarchy, uint itemId)
        {
            ThreadHelper.ThrowIfNotOnUIThread();

            int result;
            IntPtr nestedHiearchyValue = IntPtr.Zero;
            uint nestedItemIdValue = 0;
            object value = null;
            uint visibleChildNode;
            Guid nestedHierarchyGuid;
            IVsHierarchy nestedHierarchy;

            nestedHierarchyGuid = typeof(IVsHierarchy).GUID;
            result = hierarchy.GetNestedHierarchy(itemId, ref nestedHierarchyGuid, out nestedHiearchyValue, out nestedItemIdValue);

            if (result == VSConstants.S_OK && nestedHiearchyValue != IntPtr.Zero && nestedItemIdValue == VSITEMID_ROOT)
            {
                nestedHierarchy = System.Runtime.InteropServices.Marshal.GetObjectForIUnknown(nestedHiearchyValue) as IVsHierarchy;
                System.Runtime.InteropServices.Marshal.Release(nestedHiearchyValue);

                if (nestedHierarchy != null)
                    ParseHierarchy(nestedHierarchy, VSITEMID_ROOT);
            }
            else
            {
                ParseHierarchyItem(hierarchy, itemId);

                result = hierarchy.GetProperty(itemId, (int)__VSHPROPID.VSHPROPID_FirstChild, out value);

                while (result == VSConstants.S_OK && value != null)
                {
                    if (value is int && (uint)(int)value == VSITEMID_NIL)
                        break;

                    visibleChildNode = Convert.ToUInt32(value);
                    ParseHierarchy(hierarchy, visibleChildNode);

                    value = null;
                    result = hierarchy.GetProperty(visibleChildNode, (int)__VSHPROPID.VSHPROPID_NextSibling, out value);
                }
            }
        }


        private void ParseHierarchyItem(IVsHierarchy hierarchy, uint itemId)
        {
            ThreadHelper.ThrowIfNotOnUIThread();

            string canonicalName = null;
            int result = hierarchy.GetCanonicalName(itemId, out canonicalName);

            if (!string.IsNullOrEmpty(canonicalName))
            {
                if (canonicalName.EndsWith(".epoch", StringComparison.CurrentCultureIgnoreCase))
                    ParseFile(canonicalName);
            }
        }

        private void ParseFile(string filename)
        {
            string text = System.IO.File.ReadAllText(filename);
            var lexer = new LexSession(filename, text);
            var parser = new ParseSession(lexer);

            parser.AugmentProject(this);
        }

        private void ResetContents()
        {
            Files = new Dictionary<string, SourceFile>();
            GlobalScope = new LexicalScope();

            FunctionSignatures = new Dictionary<string, FunctionSignature>();
            StructureDefinitions = new Dictionary<string, Structure>();
            SumTypes = new Dictionary<string, SumType>();
            StrongAliases = new Dictionary<string, StrongAlias>();
            WeakAliases = new Dictionary<string, WeakAlias>();
        }
    }
}