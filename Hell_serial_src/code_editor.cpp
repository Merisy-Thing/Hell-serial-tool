/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>

#include "code_editor.h"

//![constructor]

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

//![constructor]

//![extraAreaWidth]

int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().width(QLatin1Char('9')) * digits;

    return space + 16;
}

//![extraAreaWidth]

//![slotUpdateExtraAreaWidth]

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

//![slotUpdateExtraAreaWidth]

//![slotUpdateRequest]

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

//![slotUpdateRequest]

//![resizeEvent]

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

bool CodeEditor::increaseSelectionIndent()
{
    QTextCursor curs = textCursor();

    if(!curs.hasSelection()) {
        return false;
    }

    // Get the first and count of lines to indent.

    int spos = curs.anchor();
    int epos = curs.position();

    if(spos > epos)
    {
        std::swap(spos, epos);
    }

    curs.setPosition(spos, QTextCursor::MoveAnchor);
    int sblock = curs.block().blockNumber();

    curs.setPosition(epos, QTextCursor::MoveAnchor);
    int eblock = curs.block().blockNumber();

    // Do the indent.

    curs.setPosition(spos, QTextCursor::MoveAnchor);

    curs.beginEditBlock();

    const int blockDifference = eblock - sblock;
    for(int i = 0; i <= blockDifference; ++i)
    {
        curs.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);

        curs.insertText("\t");

        curs.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor);
    }

    curs.endEditBlock();

    // Set our cursor's selection to span all of the involved lines.

    curs.setPosition(spos, QTextCursor::MoveAnchor);
    curs.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);

    while(curs.block().blockNumber() < eblock)
    {
        curs.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    }

    curs.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

    setTextCursor(curs);

    return true;
}
bool CodeEditor::decreaseSelectionIndent()
{
    QTextCursor curs = textCursor();

    if(!curs.hasSelection()) {
        return false;
    }

    // Get the first and count of lines to indent.

    int spos = curs.anchor();
    int epos = curs.position();

    if(spos > epos) {
        std::swap(spos, epos);
    }

    curs.setPosition(spos, QTextCursor::MoveAnchor);
    int sblock = curs.block().blockNumber();

    curs.setPosition(epos, QTextCursor::MoveAnchor);
    int eblock = curs.block().blockNumber();

    // Do the indent.

    curs.setPosition(spos, QTextCursor::MoveAnchor);

    curs.beginEditBlock();

    const int blockDifference = eblock - sblock;
    for(int i = 0; i <= blockDifference; ++i)
    {
        curs.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);

        for(int j=0; j<4; j++) {
            if(curs.atBlockEnd()) {
                break;
            }
            curs.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
            QString txt = curs.selectedText();
            if(txt == " ") {
                curs.removeSelectedText();
            } else if(txt == "\t") {
                curs.removeSelectedText();
                break;
            } else {
                break;
            }
        }

        curs.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor);
    }

    curs.endEditBlock();

    // Set our cursor's selection to span all of the involved lines.

    curs.setPosition(spos, QTextCursor::MoveAnchor);
    curs.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);

    while(curs.block().blockNumber() < eblock)
    {
        curs.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    }

    curs.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

    setTextCursor(curs);
    return true;
}

void CodeEditor::keyPressEvent ( QKeyEvent * e )
{
    switch(e->key()) {
    case Qt::Key_Tab:
        if(!increaseSelectionIndent()) {
            QPlainTextEdit::keyPressEvent(e);
        }
        break;
    case Qt::Key_Backtab:
        decreaseSelectionIndent();
        break;
    default:
        QPlainTextEdit::keyPressEvent(e);
        break;
    }
}

//![resizeEvent]

//![cursorPositionChanged]

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::yellow).lighter(160);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

//![cursorPositionChanged]

//![extraAreaPaintEvent_0]

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(0xf0, 0xf0,0xf0));

//![extraAreaPaintEvent_0]

//![extraAreaPaintEvent_1]
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();
//![extraAreaPaintEvent_1]

//![extraAreaPaintEvent_2]
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, lineNumberArea->width() - 8, fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}
//![extraAreaPaintEvent_2]

