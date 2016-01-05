/*
  This file is part of Samplecat. http://samplecat.orford.org
  copyright (C) 2007-2016 Tim Orford

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 3
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
using GLib;
using WebKit;

public class Table : GLib.Object
{
	public static WebKit.DOMDocument document;

	public WebKit.DOMElement el;
	public WebKit.DOMElement tbody;
	public WebKit.DOMElement thead;

	construct
	{
		try {
			el = document.create_element("table");
		} catch {
		};
	}

	public Table(WebKit.DOMElement parent, string id)
	{
		try {
			el.set_attribute ("id", id);
			parent.append_child ((WebKit.DOMNode)el);

			//note: append_child() returns the child
			el.append_child(thead = document.create_element ("thead"));
			el.append_child(tbody = document.create_element ("tbody"));

		} catch {
		};
	}

	public void clear()
	{
		// see also https://datatables.net/reference/api/clear()

		DOMHTMLTableElement table = (DOMHTMLTableElement)el;
		//print("rows=%lu\n", rows.length);
		try {
			for(ulong i=table.get_rows().length-1;i>=1;i--){
				table.delete_row ((long)i);
			}
		} catch {
			print("error clearing table\n");
		};
	}
}

