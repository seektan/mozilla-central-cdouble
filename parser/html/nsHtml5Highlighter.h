/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is HTML5 View Source code.
 *
 * The Initial Developer of the Original Code is
 * Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Henri Sivonen <hsivonen@iki.fi>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#ifndef nsHtml5Highlighter_h_
#define nsHtml5Highlighter_h_

#include "prtypes.h"
#include "nsCOMPtr.h"
#include "nsHtml5TreeOperation.h"
#include "nsHtml5UTF16Buffer.h"
#include "nsHtml5TreeOperation.h"
#include "nsAHtml5TreeOpSink.h"

#define NS_HTML5_HIGHLIGHTER_HANDLE_ARRAY_LENGTH 512

/**
 * A state machine for generating HTML for display in View Source based on
 * the transitions the tokenizer makes on the source being viewed.
 */
class nsHtml5Highlighter
{
  public:
    /**
     * The constructor.
     *
     * @param aOpSink the sink for the tree ops generated by this highlighter
     */
    nsHtml5Highlighter(nsAHtml5TreeOpSink* aOpSink);

    /**
     * The destructor.
     */
    ~nsHtml5Highlighter();

    /**
     * Starts the generated document.
     */
    void Start(const nsAutoString& aTitle);

    /**
     * Report a tokenizer state transition.
     *
     * @param aState the state being transitioned to
     * @param aReconsume whether this is a reconsuming transition
     * @param aPos the tokenizer's current position into the buffer
     */
    PRInt32 Transition(PRInt32 aState, bool aReconsume, PRInt32 aPos);

    /**
     * Report end of file.
     */
    void End();

    /**
     * Set the current buffer being tokenized
     */
    void SetBuffer(nsHtml5UTF16Buffer* aBuffer);

    /**
     * Let go of the buffer being tokenized but first, flush text from it.
     *
     * @param aPos the first UTF-16 code unit not to flush
     */
    void DropBuffer(PRInt32 aPos);

    /**
     * Flush the tree ops into the sink.
     *
     * @return true if there were ops to flush
     */
    bool FlushOps();

    /**
     * Linkify the current attribute value if the attribute name is one of
     * known URL attributes. (When executing tree ops, javascript: URLs will
     * not be linkified, though.)
     *
     * @param aName the name of the attribute
     * @param aValue the value of the attribute
     */
    void MaybeLinkifyAttributeValue(nsHtml5AttributeName* aName,
                                    nsString* aValue);

    /**
     * Inform the highlighter that the tokenizer successfully completed a
     * named character reference.
     */
    void CompletedNamedCharacterReference();

    /**
     * Adds an error annotation to the node that's currently on top of
     * mStack.
     *
     * @param aMsgId the id of the message in the property file
     */
    void AddErrorToCurrentNode(const char* aMsgId);

    /**
     * Adds an error annotation to the node that corresponds to the most
     * recently opened markup declaration/tag span, character reference or
     * run of text.
     *
     * @param aMsgId the id of the message in the property file
     */
    void AddErrorToCurrentRun(const char* aMsgId);

    /**
     * Adds an error annotation to the node that corresponds to the most
     * recently opened markup declaration/tag span, character reference or
     * run of text with one atom to use when formatting the message.
     *
     * @param aMsgId the id of the message in the property file
     * @param aName the atom
     */
    void AddErrorToCurrentRun(const char* aMsgId, nsIAtom* aName);

    /**
     * Adds an error annotation to the node that corresponds to the most
     * recently opened markup declaration/tag span, character reference or
     * run of text with two atoms to use when formatting the message.
     *
     * @param aMsgId the id of the message in the property file
     * @param aName the first atom
     * @param aOther the second atom
     */
    void AddErrorToCurrentRun(const char* aMsgId,
                              nsIAtom* aName,
                              nsIAtom* aOther);

    /**
     * Adds an error annotation to the node that corresponds to the most
     * recent potentially character reference-starting ampersand.
     *
     * @param aMsgId the id of the message in the property file
     */
    void AddErrorToCurrentAmpersand(const char* aMsgId);

    /**
     * Adds an error annotation to the node that corresponds to the most
     * recent potentially self-closing slash.
     *
     * @param aMsgId the id of the message in the property file
     */
    void AddErrorToCurrentSlash(const char* aMsgId);

  private:

    /**
     * Starts a span with no class.
     */
    void StartSpan();

    /**
     * Starts a <span> and sets the class attribute on it.
     *
     * @param aClass the class to set (MUST be a static string that does not
     *        need to be released!)
     */
    void StartSpan(const PRUnichar* aClass);

    /**
     * End the current <span> or <a> in the highlighter output.
     */
    void EndSpanOrA();

    /**
     * Starts a wrapper around a run of characters.
     */
    void StartCharacters();

    /**
     * Ends a wrapper around a run of characters.
     */
    void EndCharacters();

    /**
     * Starts an <a>.
     */
    void StartA();

    /**
     * Flushes characters up to but not including the current one.
     */
    void FlushChars();

    /**
     * Flushes characters up to and including the current one.
     */
    void FlushCurrent();

    /**
     * Finishes highlighting a tag in the input data by closing the open
     * <span> and <a> elements in the highlighter output and then starts
     * another <span> for potentially highlighting characters potentially
     * appearing next.
     */
    void FinishTag();

    /**
     * Adds a class attribute to the current node.
     *
     * @param aClass the class to set (MUST be a static string that does not
     *        need to be released!)
     */
    void AddClass(const PRUnichar* aClass);

    /**
     * Allocates a handle for an element.
     *
     * See the documentation for nsHtml5TreeBuilder::AllocateContentHandle()
     * in nsHtml5TreeBuilderHSupplement.h.
     *
     * @return the handle
     */
    nsIContent** AllocateContentHandle();

    /**
     * Enqueues an element creation tree operation.
     *
     * @param aName the name of the element
     * @param aAttributes the attribute holder (ownership will be taken) or
     *        nsnull for no attributes
     * @return the handle for the element that will be created
     */
    nsIContent** CreateElement(nsIAtom* aName,
                               nsHtml5HtmlAttributes* aAttributes);

    /**
     * Gets the handle for the current node. May be called only after the
     * root element has been set.
     *
     * @return the handle for the current node
     */
    nsIContent** CurrentNode();

    /**
     * Create an element and push it (its handle) on the stack.
     *
     * @param aName the name of the element
     * @param aAttributes the attribute holder (ownership will be taken) or
     *        nsnull for no attributes
     */
    void Push(nsIAtom* aName, nsHtml5HtmlAttributes* aAttributes);

    /**
     * Pops the current node off the stack.
     */
    void Pop();

    /**
     * Appends text content to the current node.
     *
     * @param aBuffer the buffer to copy from
     * @param aStart the index of the first code unit to copy
     * @param aLength the number of code units to copy
     */
    void AppendCharacters(const PRUnichar* aBuffer,
                          PRInt32 aStart,
                          PRInt32 aLength);

    /**
     * Enqueues a tree op for adding an href attribute with the view-source:
     * URL scheme to the current node.
     *
     * @param aValue the (potentially relative) URL to link to
     */
    void AddViewSourceHref(const nsString& aValue);

    /**
     * The state we are transitioning away from.
     */
    PRInt32 mState;

    /**
     * The index of the first UTF-16 code unit in mBuffer that hasn't been
     * flushed yet.
     */
    PRInt32 mCStart;

    /**
     * The position of the code unit in mBuffer that caused the current
     * transition.
     */
    PRInt32 mPos;

    /**
     * The current line number.
     */
    PRInt32 mLineNumber;

    /**
     * The number of inline elements open inside the <pre> excluding the
     * span potentially wrapping a run of characters.
     */
    PRInt32 mInlinesOpen;

    /**
     * Whether there's a span wrapping a run of characters (excluding CDATA
     * section) open.
     */
    bool mInCharacters;

    /**
     * The current buffer being tokenized.
     */
    nsHtml5UTF16Buffer* mBuffer;

    /**
     * Whether to highlight syntax visibly initially.
     */
    bool mSyntaxHighlight;

    /**
     * The outgoing tree op queue.
     */
    nsTArray<nsHtml5TreeOperation> mOpQueue;

    /**
     * The tree op stage for the tree op executor.
     */
    nsAHtml5TreeOpSink* mOpSink;

    /**
     * The most recently opened markup declaration/tag or run of characters.
     */
    nsIContent** mCurrentRun;

    /**
     * The most recent ampersand in a place where character references were
     * allowed.
     */
    nsIContent** mAmpersand;

    /**
     * The most recent slash that might become a self-closing slash.
     */
    nsIContent** mSlash;

    /**
     * Memory for element handles.
     */
    nsAutoArrayPtr<nsIContent*> mHandles;

    /**
     * Number of handles used in mHandles
     */
    PRInt32 mHandlesUsed;

    /**
     * A holder for old contents of mHandles
     */
    nsTArray<nsAutoArrayPtr<nsIContent*> > mOldHandles;

    /**
     * The element stack.
     */
    nsTArray<nsIContent**> mStack;

    /**
     * The string "comment"
     */
    static PRUnichar sComment[];

    /**
     * The string "cdata"
     */
    static PRUnichar sCdata[];

    /**
     * The string "start-tag"
     */
    static PRUnichar sStartTag[];

    /**
     * The string "attribute-name"
     */
    static PRUnichar sAttributeName[];

    /**
     * The string "attribute-value"
     */
    static PRUnichar sAttributeValue[];

    /**
     * The string "end-tag"
     */
    static PRUnichar sEndTag[];

    /**
     * The string "doctype"
     */
    static PRUnichar sDoctype[];

    /**
     * The string "entity"
     */
    static PRUnichar sEntity[];

    /**
     * The string "pi"
     */
    static PRUnichar sPi[];
};

#endif // nsHtml5Highlighter_h_
