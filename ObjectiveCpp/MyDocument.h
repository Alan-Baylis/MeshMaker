//
//  MyDocument.h
//  OpenGLEditor
//
//  Created by Filip Kunc on 6/29/09.
//  For license see LICENSE.TXT
//

#import <Cocoa/Cocoa.h>
#import "ItemCollection.h"
#import "OpenGLSceneView.h"
#import "AddItemWithStepsSheetController.h"
#import "MeshManipulationState.h"
#import "PropertyReflector.h"
#import "TexturePaintView.h"

@interface MyDocument : NSDocument <AddItemWithStepsProtocol, OpenGLSceneViewDelegate, OpenGLTransforming, NSTableViewDataSource, NSTableViewDelegate, TexturePaintViewDelegate>
{
@public // public for unit tests
	BOOL manipulationFinished;
	ItemCollection *items;
	OpenGLManipulatingController *itemsController;
	OpenGLManipulatingController *meshController;
	id<OpenGLManipulating> manipulated;
	IBOutlet OpenGLSceneView *viewLeft;
	IBOutlet OpenGLSceneView *viewTop;
	IBOutlet OpenGLSceneView *viewFront;
	IBOutlet OpenGLSceneView *viewPerspective;
	IBOutlet NSPopUpButton *editModePopUp;
	IBOutlet NSPopUpButton *viewModePopUp;
	IBOutlet AddItemWithStepsSheetController *addItemWithStepsSheetController;
	enum MeshType itemWithSteps;
	
	NSMutableArray *views;
	NSMutableArray *oldManipulations;
	MeshManipulationState *oldMeshManipulation;
	
	IBOutlet NSSplitView *topSplit;
	IBOutlet NSSplitView *bottomSplit;
	IBOutlet NSSplitView *mainSplit;
	OpenGLSceneView *oneView;
	
	PropertyReflector *propertyReflector;
	IBOutlet NSTableView *propertyView;
	IBOutlet NSTableView *objectView;

	IBOutlet TexturePaintView *texturePaintView;
}

@property (readwrite, assign) id<OpenGLManipulating> manipulated;

- (void)setNeedsDisplayExceptView:(OpenGLSceneView *)view;
- (void)setNeedsDisplayOnAllViews;
- (void)syncObjectView;
- (Mesh *)currentMesh;
- (MyDocument *)prepareUndoWithName:(NSString *)actionName;
- (void)swapManipulationsWithOld:(NSMutableArray *)old current:(NSMutableArray *)current;
- (void)swapMeshManipulationWithOld:(MeshManipulationState *)old current:(MeshManipulationState *)current;
- (void)swapAllItemsWithOld:(NSMutableArray *)old
					current:(NSMutableArray *)current
				 actionName:(NSString *)actionName;
- (void)swapMeshFullStateWithOld:(MeshFullState *)old 
						 current:(MeshFullState *)current 
					  actionName:(NSString *)actionName;
- (void)allItemsActionWithName:(NSString *)actionName block:(void (^blockmethod)())action;
- (void)fullMeshActionWithName:(NSString *)actionName block:(void (^blockmethod)())action;
- (void)addItemWithType:(enum MeshType)type steps:(uint)steps;
- (void)removeItemWithType:(enum MeshType)type steps:(uint)steps;
- (IBAction)addCube:(id)sender;
- (IBAction)addCylinder:(id)sender;
- (IBAction)addSphere:(id)sender;
- (void)editMeshWithMode:(enum MeshSelectionMode)mode;
- (void)editItems;
- (IBAction)changeEditMode:(id)sender;
- (IBAction)changeManipulator:(id)sender;
- (IBAction)changeViewMode:(id)sender;
- (IBAction)mergeSelected:(id)sender;
- (IBAction)splitSelected:(id)sender;
- (IBAction)mergeVertexPairs:(id)sender;
- (IBAction)cloneSelected:(id)sender;
- (void)redoCloneSelected:(NSMutableArray *)selection;
- (void)undoCloneSelected:(NSMutableArray *)selection;
- (IBAction)flipSelected:(id)sender;
- (IBAction)deleteSelected:(id)sender;
- (void)redoDeleteSelected:(NSMutableArray *)selectedItems;
- (void)undoDeleteSelected:(NSMutableArray *)selectedItems;
- (IBAction)selectAll:(id)sender;
- (IBAction)invertSelection:(id)sender;
- (BOOL)readFromModel3D:(NSString *)fileName;
- (void)writeToModel3D:(NSString *)fileName;
- (void)toggleOneViewFourView:(id)sender;

@end
