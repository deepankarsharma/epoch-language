﻿using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio.TextManager.Interop;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class ParseSession
    {

        private LexSession Lexer;
        private ErrorListProvider ErrorProvider;



        internal class ErrorListHelper : IServiceProvider
        {
            public object GetService(Type serviceType)
            {
                return Package.GetGlobalService(serviceType);
            }
        }


        public ParseSession(LexSession lexer)
        {
            Lexer = lexer;

            var helper = new ErrorListHelper();
            ErrorProvider = new ErrorListProvider(helper);
            ErrorProvider.ProviderName = "Epoch Language";
            ErrorProvider.ProviderGuid = new Guid(VsPackage.PackageGuid);
        }

        public bool AugmentProject(Project project)
        {
            try
            {
                while (!Lexer.Empty)
                {
                    var sumtype = SumType.Parse(this);
                    if (sumtype != null)
                    {
                        project.RegisterSumType(sumtype.Name, sumtype.Type);
                        continue;
                    }



                    // TODO - this should be come syntax error data when parser is fleshed out
                    throw new Exception("This syntax is not parsed yet");


                    /*
                    if (ParseSumType(filename, tokens))
                    {
                    }
                    else if (ParseStrongAlias(filename, tokens))
                    {
                    }
                    else if (ParseWeakAlias(filename, tokens))
                    {
                    }
                    else if (ParseStructure(filename, tokens))
                    {
                    }
                    else if (ParseGlobalBlock(filename, tokens))
                    {
                    }
                    else if (ParseTask(filename, tokens))
                    {
                    }
                    else if (ParseFunction(filename, tokens))
                    {
                    }
                    */
                }
            }
            catch (Exception ex)
            {
                var errorTask = new ErrorTask();
                errorTask.Text = ex.Message;
                errorTask.Category = TaskCategory.CodeSense;
                errorTask.ErrorCategory = TaskErrorCategory.Error;
                errorTask.Document = Lexer.File;
                errorTask.Line = (!Lexer.Empty && Lexer.PeekToken(0) != null) ? (Lexer.PeekToken(0).Line) : 0;
                errorTask.Column = (!Lexer.Empty && Lexer.PeekToken(0) != null) ? (Lexer.PeekToken(0).Column) : 0;
                errorTask.Navigate += NavigationHandler;

                ErrorProvider.Tasks.Add(errorTask);
                return false;
            }

            return true;
        }

        internal bool CheckToken(int offset, string expected)
        {
            if (Lexer.Empty)
                return false;

            var token = Lexer.PeekToken(offset);
            if (token == null)
                return false;

            return token.Text.Equals(expected);
        }

        internal void ConsumeTokens(int count)
        {
            Lexer.ConsumeTokens(count);
        }

        internal Token PeekToken(int offset)
        {
            if (Lexer.Empty)
                return null;

            return Lexer.PeekToken(offset);
        }


        internal bool ParseTemplateParameters(int startoffset, Token basetypename, out int totaltokens)
        {
            totaltokens = startoffset;
            bool hasparams = true;

            while (hasparams)
            {
                totaltokens += 2;

                if (CheckToken(totaltokens, ">"))
                {
                    ++totaltokens;
                    hasparams = false;
                }
                else if (CheckToken(totaltokens, ","))
                    ++totaltokens;
                else
                    return false;
            }

            return true;
        }

        internal bool ParseTemplateArguments(int startoffset, Token basetypename, out int totaltokens)
        {
            totaltokens = startoffset;
            bool hasargs = true;

            while (hasargs)
            {
                ++totaltokens;

                if (CheckToken(totaltokens, ">"))
                    hasargs = false;
                else if (CheckToken(totaltokens, ","))
                    ++totaltokens;
                else
                    return false;

            }

            return true;
        }


        internal async void NavigationHandler(object sender, EventArgs args)
        {
            var task = sender as Microsoft.VisualStudio.Shell.Task;
            if (string.IsNullOrEmpty(task.Document))
                return;

            await ThreadHelper.JoinableTaskFactory.SwitchToMainThreadAsync();

            var serviceProvider = new ErrorListHelper();
            var openDoc = serviceProvider.GetService(typeof(IVsUIShellOpenDocument)) as IVsUIShellOpenDocument;
            if (openDoc == null)
                return;

            IVsWindowFrame frame;
            Microsoft.VisualStudio.OLE.Interop.IServiceProvider sp;
            IVsUIHierarchy hier;
            uint itemid;
            Guid logicalView = VSConstants.LOGVIEWID_Code;

            if (ErrorHandler.Failed(openDoc.OpenDocumentViaProject(task.Document, ref logicalView, out sp, out hier, out itemid, out frame)) || frame == null)
                return;

            object docData;
            frame.GetProperty((int)__VSFPROPID.VSFPROPID_DocData, out docData);

            VsTextBuffer buffer = docData as VsTextBuffer;
            if (buffer == null)
            {
                IVsTextBufferProvider bufferProvider = docData as IVsTextBufferProvider;
                if (bufferProvider != null)
                {
                    IVsTextLines lines;
                    ErrorHandler.ThrowOnFailure(bufferProvider.GetTextBuffer(out lines));
                    buffer = lines as VsTextBuffer;

                    if (buffer == null)
                        return;
                }
            }

            IVsTextManager mgr = serviceProvider.GetService(typeof(VsTextManagerClass)) as IVsTextManager;
            if (mgr == null)
                return;

            // TODO - this for some reason loads our code as JSON!
            mgr.NavigateToLineAndColumn(buffer, ref logicalView, task.Line, task.Column, task.Line, task.Column);
        }
    }
}
