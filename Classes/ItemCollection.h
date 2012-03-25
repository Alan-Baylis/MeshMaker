//
//  ItemCollection.h
//  OpenGLEditor
//
//  Created by Filip Kunc on 6/28/09.
//  For license see LICENSE.TXT
//

#import <Cocoa/Cocoa.h>
#import "Item.h"
#import "OpenGLSelecting.h"
#import "OpenGLManipulating.h"
#import "OpenGLManipulatingController.h"
#import "IndexedItem.h"
#import "MeshState.h"

@interface ItemCollection : NSObject <OpenGLManipulatingModelItem, MemoryStreaming>
{
	NSMutableArray *items;
}

@property (readwrite, assign) NSMutableArray *currentManipulations;
@property (readwrite, assign) MeshState *currentMeshState;
@property (readwrite, assign) NSMutableArray *currentSelection;
@property (readwrite, assign) NSMutableArray *currentItems;
@property (readwrite, retain) NSMutableArray *allItems;

- (Item *)itemAtIndex:(uint)index;
- (void)addItem:(Item *)item;
- (void)removeItem:(Item *)item;
- (void)removeLastItem;
- (void)removeItemAtIndex:(uint)index;
- (void)removeItemsInRange:(NSRange)range;
- (void)insertItem:(Item *)item atIndex:(uint)index;
- (void)mergeSelectedItems;
- (void)setSelectionFromIndexedItems:(NSMutableArray *)anItems;
- (void)deselectAll;
- (void)getVertexCount:(uint *)vertexCount triangleCount:(uint *)triangleCount;
- (Mesh *)currentMesh;
- (Item *)firstSelectedItem;

@end
