#! /usr/bin/env python

import re
import sys
import os.path

# We maintain a small state machine.
FIRST = 1
TEXT = 2
ARGS = 3
RET = 4
RAISES = 5
NOTE = 6
TODO = 7

class Section:
    def __init__(self):
        self.first = ""
        self.text = ""
        self.args = []
        self.returns = []
        self.raises = []
        self.notes = []
        self.todos = []
        
        self.state = FIRST

re_arg = re.compile(r"^\s*([a-zA-Z0-9_]+):\s+(([^-]+)-\s+)?(.*)$")
re_returns = re.compile(r"^\s*Returns:\s*(([^-]+)-\s*)?(.*)$")
re_raises = re.compile(r"^\s*Raises:\s*([^-]+)-\s*(.*)$")
re_note = re.compile(r"^\s*Note:\s*(.*)$")
re_todo = re.compile(r"^\s*To-?[dD]o:\s*(.*)$")

re_skip_first = re.compile(r"^\s*[\\-]")

re_include = re.compile(r"^\s*Include:\s*(.*)$")

fill_in_args = {}

def readFillInArgs():
    try:
        file = os.path.join(os.path.dirname(sys.argv[0]), "default-args.txt")
        for line in open(file):
            
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            
            m = line.split()
            fill_in_args[m[0]] = " ".join(m[1:])
            
    except IOError:
        pass

def feedLine(sec, line):
    if sec.state == FIRST:

        m = re_skip_first.match(line)
        if m:
            sec.state = TEXT
        else:
            i = line.find(".")
            if i < 0:
                i = line.find(":")
            
            if i < 0:
                sec.first += line + "\n"
            else:
                sec.first += line[0:i+1] + "\n"
                sec.text += line[i+1:].strip() + "\n"
                sec.state = TEXT
            return

    next_state = sec.state

    m = None
    for (regexp, nst) in [(re_returns, RET), (re_raises, RAISES), (re_todo, TODO), (re_note, NOTE), (re_arg, ARGS)]:
        m = regexp.match(line)
        if m:
            next_state = nst
            break
        
    if not m:
        # Continue current section.
        if sec.state == TEXT:
            sec.text += " " + line  + "\n"
            
        elif sec.state == ARGS:
            if line.strip():
                sec.args[-1][-1] += " " + line.strip()
            
        elif sec.state == RET:
            if line.strip():
                sec.returns[-1][-1] += " " + line.strip()
                
        elif sec.state == RAISES:
            if line.strip():
                sec.raises[-1][-1] += " " + line.strip()
            
        elif sec.state == NOTE:
            sec.notes[-1] += " " + line.strip()
            
        elif sec.state == TODO:
            sec.todos[-1] += " " + line.strip()
            
        else:
            assert False

    else:
        # Start new section.
        if next_state == ARGS:
            (id, type, descr) = (m.group(1), m.group(2), m.group(4))
            if type:
                type = type.strip()
            else:
                type = ""
                
            if id in fill_in_args and descr.strip() == "&":
                descr = fill_in_args[id]
                
            sec.args += [[type, id.strip(), descr.strip()]]
            
        if next_state == RET:
            (type, descr) = (m.group(1), m.group(3))
            if type:
                type = type.strip()
            else:
                type = ""
            sec.returns += [[type, descr.strip()]]
            
        if next_state == RAISES:
            (type, descr) = (m.group(1), m.group(2))
            if type:
                type = type.strip()
            else:
                type = ""
            sec.raises += [[type, descr.strip()]]
            
        if next_state == NOTE:
            txt = m.group(1)
            sec.notes += [txt]
            
        if next_state == TODO:
            txt = m.group(1)
            newline = txt
            sec.todos += [txt]
        
        sec.state = next_state
    
def expandMarkup(line):
    return line.replace("~~", "::")
        
def expandBlock(lines):
    sec = Section()

    for line in lines:
        m = re_include.match(line)
        if m:
            file = m.groups()[0]
            try:
                file = os.path.join(os.path.dirname(sys.argv[0]), file)
                
                for incl in open(file):
                    feedLine(sec, incl.strip())
            except IOError:
                sec.text += " <Cannot open include file %s>" % file
            
        else:
            feedLine(sec, line)

    return sec

readFillInArgs()
