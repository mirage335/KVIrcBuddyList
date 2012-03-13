//KVIrc script. Basically extends the 'buddy list' concept from IM to IRC for technical users.

// Copyright (c) 2012 mirage335

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

//Holds functions to query IRC server, detect replies, and clean up event handlers.
class(contactProcessor,object) {
	//Send raw ison command to server. The response must be trapped asynchronyously with an event handler.
	queryIsOn(<$0 = contactName>,<$1 = network>,<$2 = items>) {
		foreach(%ic,$context.list) {
			if ($context.networkName(%ic) == "$1") {
				%winid = $window
				rebind %ic
				
				raw ison "$0"
				
				rebind %winid
				
				//Abort once a match has been found and query has been issued.
				return
			}
		}
		//If we reached here, network is not available for query.
		$2->%processedItems++
	}
	//Creates new event handler to trap ANY ison response, and increment processed items counter. Should only be called ONCE.
	trapIsOn(<$0 = items>) {
		%MustPassToStupidEventHandlerSomehow8b6063b715404848834befc427f2f3be = $0
		event(303,"BuddyListContactProcessorIsOnTrap") {
			%items = %MustPassToStupidEventHandlerSomehow8b6063b715404848834befc427f2f3be
			if ("$3" != "")
				%items->$markContactOnline($context.networkName,"$3")
			%items->%processedItems++
			
			%items->$attemptOutput
		}
	}
	//Set this command to run after all ison statements have returned an answer (and all items have been processed).
	deleteTrap() {
		event(303,"BuddyListContactProcessorIsOnTrap") {}
		unset %MustPassToStupidEventHandlerSomehow8b6063b715404848834befc427f2f3be
	}
}

//Buddy object, retrieves, stores, and writes out contact information.
class(contact,object) {
	//Function is called upon object creation.
	constructor() {
		$this->%nickname=""
		$this->%network=""
		$this->%notes=""
		$this->%online=Off
		$this->%metaID=""
		$this->%metaWritten=false
	}
	//Sends ison command to server.
	process(<$0 = items>) {
		%contactProcessor = $new(contactProcessor)
		%contactProcessor->$queryIsOn($this->%nickname,$this->%network,$0)
	}
	//Writes HTML code to file based on buddy contents.
	write() {
		if ($this->%online == On)
			%contactStatusColor = teal
		else
			%contactStatusColor = grey
		if($this->%metaID == "")
			file.write -a $file.localdir(KVIrcBuddyList.html) "\<tr\>\<td bgcolor\=%contactStatusColor\>$this->%online\</td\>\<td\>$this->%nickname\</td\>\<td\>$this->%network\</td\>\<td\>$this->%notes\</td\>\</tr\>\n"
		else
			file.write -a $file.localdir(KVIrcBuddyList.html) "\<tr\>\<td bgcolor\=%contactStatusColor\>$this->%online\</td\>\<td\>$this->%nickname\</td\>\<td\>MetaContacts\</td\>\<td\>$this->%notes\</td\>\</tr\>\n"
	}
}

//Special type of object. Similar to contact object, but for raw HTML code. Useful for table headers/footers.
//Class requires input:
//$this->%content
class(HTMLinsert,object) {
	constructor() {
		$this->%processed = false
		$this->%content = ""
		$this->%metaID=""
		$this->%metaWritten=false
		$this->%online=Off
	}
	process(<$0 = items>) {
		//HTML objects don't need processing, so just go straight to the output step.
		$this->%processed = true
		$0->%processedItems++
		$0->$attemptOutput
	}
	write() {
		file.write -a $file.localdir(KVIrcBuddyList.html) $this->%content
	}
}

//Includes and manages all items in a single array.
class(items,object) {
	//Function is called upon object creation.
	constructor() {
		$this->%index = 0
		$this->%processedItems = 0
	}
	//Adds buddy to items.
	addContact(<$0 = network, $1 = nickname, $2 = notes, $3 = metaID>) {
	if ("$0" == "")
		return
	if ("$1" == "")
		return
	$this->%allItems[$this->%index] = $new(contact)
	$this->%allItems[$this->%index]->%network ="$0"
	$this->%allItems[$this->%index]->%nickname = "$1"
	if ("$2" != "")
		$this->%allItems[$this->%index]->%notes = "$2"
	if ("$3" != "")
		$this->%allItems[$this->%index]->%metaID = "$3"
	$this->%index++
	}
	addHTMLinsert(<$0 = content>) {
		$this->%allItems[$this->%index] = $new(HTMLinsert)
		if ($0 != "")
			$this->%allItems[$this->%index]->%content = $0
		$this->%index++
	}
	//Process all items. For buddies, this means sending ison command with global trap.
	processItems() {
		%contactProcessor = $new(contactProcessor)
		%contactProcessor->$trapIsOn($this)
		foreach(%item,$this->%allItems)
			%item->$process($this)
	}
	markContactOnline(<$0 = network, $1 = nickname) {
		foreach(%item,$this->%allItems) {
			if ("$1" == "%item->%nickname ")
				if("$0" == "%item->%network")
					%item->%online=On
			if ("$1" == "%item->%nickname")
				if("$0" == "%item->%network")
					%item->%online=On
			}
	}
	attemptOutput() {
		if($this->%processedItems < $this->%index)
			return
		file.write $file.localdir(KVIrcBuddyList.html) "\<html\>\n  \<head\>\n    \<title\>buddyList\</title\>\n  \</head\>\n  \<body\>\n"
		foreach(%item,$this->%allItems) {
			//If subordinate metacontact items have been found online, update this one.
			if(%item->%metaID != "")
				foreach(%otherItem,$this->%allItems)
					if(%item->%metaID == %otherItem->%metaID)
						if(%otherItem->%online == "On")
							%item->%online="On"
			if(%item->%metaWritten == false)
				%item->$write
			//Set metaWritten true when a previous item with same metaID has given output. 
			if(%item->%metaID != "")
				foreach(%otherItem,$this->%allItems)
					if(%item->%metaID == %otherItem->%metaID)
						%otherItem->%metaWritten = true
		}
		file.write -a $file.localdir(KVIrcBuddyList.html) "  \</body\>\n\</html\>\n"
		$this->$display
		%contactProcessor = $new(contactProcessor)
		%contactProcessor->$deleteTrap()
	}
	display() {
		//Thanks to TheRipper from #kvirc on freenode for suggesting the following webview.
		%d = $new(dockwindow);
		%x = $new(webview,%d);
		%d->$addWidget(%x);
		%d->$dock(r);
		%x->$load("$file.localdir(KVIrcBuddyList.html)");
		%d->$show;
	}
}

%items = $new(items)

//Feel free to include images using the data URI scheme.
%items->$addHTMLinsert("\<b\>Buddies:\</b\>\<br\>\<table\ border\=1>\n\<tr\>\n\<th\>Status\</th\>\n<th\>Nick\</th\>\n\<th\>Network\</th\>\n\<th\>Notes\</th\>\n\</tr\>\n")
%items->$addContact(netName,buddyName,"Notes")
%items->$addContact(netName,doublyAvailableBuddy,"Notes","metaContactName")
%items->$addContact(otherNetName,doublyAvailableBuddy,"Notes","metaContactName")
%items->$addHTMLinsert("\</table\>\n")

%items->$processItems