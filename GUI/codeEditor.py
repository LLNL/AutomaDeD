##############################################################################
# Copyright (c) 2014, Lawrence Livermore National Security, LLC.
# Produced at the Lawrence Livermore National Laboratory.
#
# This file is part of AutomaDeD.
# Written by:
# Ignacio Laguna:lagunaperalt1@llnl.gov, Subrata Mitra: mitra4@purdue.edu.
# All rights reserved.
# LLNL-CODE-647182
#
# For details, see https://github.com/scalability-llnl/AutomaDeD
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License (as published by
# the Free Software Foundation) version 2.1 dated February 1999.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms and
# conditions of the GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
##############################################################################
#from PyQt4.Qt import QFrame
#from PyQt4.Qt import QHBoxLayout
#from PyQt4.Qt import QPainter
#from PyQt4.Qt import QPlainTextEdit
#from PyQt4.Qt import QRect
#from PyQt4.Qt import QTextEdit
#from PyQt4.Qt import QTextFormat
#from PyQt4.Qt import QVariant
#from PyQt4.Qt import QWidget
#from PyQt4.Qt import Qt

from PyQt4.QtCore import *
#from PySide.QtCore import *
from PyQt4.QtGui import *
#from PySide.QtGui import *
 
 
class NumberBar(QWidget):
 
        def __init__(self, edit):
            QWidget.__init__(self, edit)
 
            self.edit = edit
            self.adjustWidth(1)
 
        def paintEvent(self, event):
            self.edit.numberbarPaint(self, event)
            QWidget.paintEvent(self, event)
 
        def adjustWidth(self, count):
            width = self.fontMetrics().width(unicode(count))
            if self.width() != width:
                self.setFixedWidth(width+1)
 
        def updateContents(self, rect, scroll):
            if scroll:
                self.scroll(0, scroll)
            else:
                # It would be nice to do
                # self.update(0, rect.y(), self.width(), rect.height())
                # But we can't because it will not remove the bold on the
                # current line if word wrap is enabled and a new block is
                # selected.
                self.update()
 
 
class PlainTextEdit(QPlainTextEdit):
 
        def __init__(self, *args):
            QPlainTextEdit.__init__(self, *args)
 	    self.maxLineNumber = 1
            #self.setFrameStyle(QFrame.NoFrame)
 
            self.setFrameStyle(QFrame.NoFrame)
            self.highlight()
            #self.setLineWrapMode(QPlainTextEdit.NoWrap)
 
            self.cursorPositionChanged.connect(self.highlight)

	def setMaxLineNumber(self,num_lines):
	    self.maxLineNumber = num_lines

        def highlight(self):
            hi_selection = QTextEdit.ExtraSelection()
 
            hi_selection.format.setBackground(QColor(Qt.yellow).lighter(160))
            hi_selection.format.setProperty(QTextFormat.FullWidthSelection, QVariant(True))
            hi_selection.cursor = self.textCursor()
            hi_selection.cursor.clearSelection()
 
            self.setExtraSelections([hi_selection])
	
	def highlight_lines(self, lines, current_line):
            all_selections = []
	    first_line = lines[0]
            for line in lines:
                    hi_selection = QTextEdit.ExtraSelection()
                    if line == current_line:
                            hi_selection.format.setBackground(QColor(Qt.green).lighter(160))
                    else:
                            hi_selection.format.setBackground(QColor(Qt.red).lighter(160))
                    hi_selection.format.setProperty(QTextFormat.FullWidthSelection, QVariant(True))
                    hi_selection.cursor = self.textCursor()
                    hi_selection.cursor.setPosition(0, QTextCursor.MoveAnchor)
                    hi_selection.cursor.movePosition(QTextCursor.Down, QTextCursor.MoveAnchor,line-1)
                    hi_selection.cursor.clearSelection()
                    all_selections.append(hi_selection)

            self.setExtraSelections(all_selections)
	    #line_width = self.lineWidth()
	    #line_y = line_width * first_line
	    #line_x = 10
	    #self.move(line_x,line_y)
	    max = self.verticalScrollBar().maximum()
	    min = self.verticalScrollBar().minimum()
	    pos = min + first_line*(float(max - min)/float(self.maxLineNumber))
	    #pos = 35
	    #print "firstline" , first_line, " pos:", pos, " minimum is", min, " maximum is ", max
	    self.verticalScrollBar().setValue(pos)
 
        def numberbarPaint(self, number_bar, event):
            font_metrics = self.fontMetrics()
            current_line = self.document().findBlock(self.textCursor().position()).blockNumber() + 1
 
            block = self.firstVisibleBlock()
            line_count = block.blockNumber()
            painter = QPainter(number_bar)
            painter.fillRect(event.rect(), self.palette().base())
 
            # Iterate over all visible text blocks in the document.
            while block.isValid():
                line_count += 1
                block_top = self.blockBoundingGeometry(block).translated(self.contentOffset()).top()
 
                # Check if the position of the block is out side of the visible
                # area.
                if not block.isVisible() or block_top >= event.rect().bottom():
                    break
 
                # We want the line number for the selected line to be bold.
                if line_count == current_line:
                    font = painter.font()
                    font.setBold(True)
                    painter.setFont(font)
                else:
                    font = painter.font()
                    font.setBold(False)
                    painter.setFont(font)
 
                # Draw the line number right justified at the position of the line.
                paint_rect = QRect(0, block_top, number_bar.width(), font_metrics.height())
                painter.drawText(paint_rect, Qt.AlignRight, unicode(line_count))
 
                block = block.next()
 
            painter.end()
 
