// ComplexAtribute.h: interface for the CComplexAtribute class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COMPLEXATRIBUTE_H__8E851026_E0ED_11D2_9713_0020AFA6CCC8__INCLUDED_)
#define AFX_COMPLEXATRIBUTE_H__8E851026_E0ED_11D2_9713_0020AFA6CCC8__INCLUDED_
/*___INFO__MARK_BEGIN__*/
/*************************************************************************
 * 
 *  The Contents of this file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 * 
 *  Sun Microsystems Inc., March, 2001
 * 
 * 
 *  Sun Industry Standards Source License Version 1.2
 *  =================================================
 *  The contents of this file are subject to the Sun Industry Standards
 *  Source License Version 1.2 (the "License"); You may not use this file
 *  except in compliance with the License. You may obtain a copy of the
 *  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
 * 
 *  Software provided under this License is provided on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
 *  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
 *  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
 *  See the License for the specific provisions governing your rights and
 *  obligations concerning the Software.
 * 
 *   The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 * 
 *   Copyright: 2001 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <deque>

#include "SgeObj.h"

extern "C" {
#include "cull_list.h"
}

class CComplexAtribute : public CSgeObj  
{
public:
	CString name;
	CString shortcut;
	CString stringval;
	CString defaultval;        // CE_default
	CString pj_stringval;
	ULONG   valtype;
	ULONG   relop;
	ULONG   request;
	ULONG   consumable;
	ULONG   forced;
	ULONG   dominant;
	ULONG   pj_dominant;
	DOUBLE  doubleval;
	DOUBLE  pj_doubleval;

	CComplexAtribute();
	CComplexAtribute(lListElem *qep);
	virtual ~CComplexAtribute();

	operator lListElem* ();
};

class CComplexAtributeList : public std::deque<CComplexAtribute>    
{
public:
	CComplexAtributeList::iterator FindByID(ULONG ID);
	CComplexAtributeList::iterator FindByName(const CString &Name);
	void RemoveByID(ULONG ID);
	void DebugOut();
	lList *MakelList();
};

#endif // !defined(AFX_COMPLEXATRIBUTE_H__8E851026_E0ED_11D2_9713_0020AFA6CCC8__INCLUDED_)
