/*
 iAppTableView
 Written by Evan Jones <ejones@uwaterloo.ca>, 27-03-2003
 http://www.eng.uwaterloo.ca/~ejones/

 Released under the GNU LGPL.

 That means that you can use this class in open source or commercial products, with the limitation that you must distribute the source code for this class, and any modifications you make. See http://www.gnu.org/ for more information.
 */

#import "ColoredRowOutlineView.h"


@implementation ColoredRowOutlineView

- (void)initialize
{
    NSSize spacing = _intercellSpacing;
    NSArray* columns = _tableColumns;
    unsigned int i = 0;

    spacing.height = 0.0;
    [self setIntercellSpacing: spacing];

    for ( i = 0; i < [columns count]; i ++ ) {
        //NSLog( @"drawsBackground: %d", [[[columns objectAtIndex: i] dataCell] drawsBackground] );
        id cell = [[columns objectAtIndex: i] dataCell];
        if ( [cell respondsToSelector:@selector(setDrawsBackground:)] ) {
            [cell setDrawsBackground: NO];
        }
    }
}

- (id)initWithFrame: (NSRect)frame {
    id ptr = [super initWithFrame: frame];
    if ( ptr != nil ) {
        [self initialize];
    }
    return ptr;
}

- (void)awakeFromNib {
    [self initialize];
}

- (void) drawRect:(NSRect) r{
    NSRect 	rowRect, nextRect;
    NSSize	cellSpacing;
    int		countRows, row=0;
    float	height;
    BOOL	isOddRow;
    float	firstBlankRowYPos;
    NSColor	*rowColor;
    // draw the populated rows
    [super drawRect:r];
    
    // draw remaining lines in blank area
    for(countRows=0;row!=-1;countRows++) {
        row=[self levelForRow:countRows];
    }
    countRows--;
    rowRect = [self rectOfRow:countRows-1];
    cellSpacing = _intercellSpacing;
    height = _rowHeight + cellSpacing.height;
    nextRect.origin.x = r.origin.x;
    nextRect.origin.y = rowRect.origin.y+height;
    nextRect.size.width = r.size.width;
    nextRect.size.height=height;
    if (countRows==0) {
        isOddRow = TRUE;
        nextRect.origin.y = 0;
    }
    else
        isOddRow = ((countRows%2)==0); // next row is odd if last row is even
    firstBlankRowYPos = nextRect.origin.y;
    row = countRows;
    while (nextRect.origin.y<(r.origin.y+r.size.height)) {
        if (isOddRow)
            rowColor = [NSColor colorWithCalibratedRed: 0.92941 green: 0.95294 blue: 0.99607 alpha: 1.0];
        else
            rowColor = _backgroundColor;
        [rowColor set];
        [NSBezierPath fillRect:nextRect];
        isOddRow = !isOddRow;
        nextRect.origin.y += height;
        row++;
    }
}

- (void)drawRow:(int)rowIndex clipRect:(NSRect)clipRect {
    //NSLog( @"Drawing: %d", rowIndex );

    if ( rowIndex % 2 == 0 && ! [self isRowSelected: rowIndex] ) {
        NSRect rowBounds = [self rectOfRow: rowIndex];

        NSBezierPath* path = [NSBezierPath bezierPathWithRect: rowBounds];
        [[NSColor colorWithCalibratedRed: 0.92941 green: 0.95294 blue: 0.99607 alpha: 1.0] set];
        [path fill];
    }
            
    [super drawRow: rowIndex clipRect: clipRect];
}

@end
