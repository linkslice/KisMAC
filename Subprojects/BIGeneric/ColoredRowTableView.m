/*
 iAppTableView
 Written by Evan Jones <ejones@uwaterloo.ca>, 27-03-2003
 http://www.eng.uwaterloo.ca/~ejones/

 Released under the GNU LGPL.

 That means that you can use this class in open source or commercial products, with the limitation that you must distribute the source code for this class, and any modifications you make. See http://www.gnu.org/ for more information.
 */

#import "ColoredRowTableView.h"


@implementation ColoredRowTableView

- (void)initialize
{
    NSArray* columns = _tableColumns;
    NSSize spacing = _intercellSpacing;
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
    int		countRows, row;
    float	height;
    BOOL	isOddRow;
    float	firstBlankRowYPos;
    NSColor	*rowColor;
    // draw the populated rows
    [super drawRect:r];
    // draw remaining lines in blank area
    countRows = [_dataSource numberOfRowsInTableView:self];
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
    if (([self drawsGrid]) && (firstBlankRowYPos<(r.origin.y+r.size.height))) {
        NSArray 	*tcs = _tableColumns;
        unsigned int 	i;
        int 		xPos = -r.origin.x; // if horizontal scrolling occurred
        float interspacingWidth = _intercellSpacing.width;
        // Drawing grid in blank area
        for (i=0;i<([tcs count]-1);i++) { // no grid line after last column
            xPos += [[tcs objectAtIndex:i] width] + interspacingWidth;
            if (xPos > 0) // skip columns until they are viewable
                if(xPos <= r.origin.x + r.size.width) {
                    [_gridColor set];
                    [NSBezierPath setDefaultLineWidth:1.0];
                    [NSBezierPath strokeLineFromPoint:NSMakePoint(r.origin.x - 0.5 + xPos,firstBlankRowYPos)
                            toPoint:NSMakePoint(r.origin.x + xPos - 0.5,r.origin.y + r.size.height)];
                }
                else
                    break;
        }
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