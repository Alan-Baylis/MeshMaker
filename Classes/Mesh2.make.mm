//
//  Mesh2.make.cpp
//  OpenGLEditor
//
//  Created by Filip Kunc on 19.06.11.
//  For license see LICENSE.TXT
//

#include "Mesh2.h"

void Mesh2::addTriangle(VertexNode *v1, VertexNode *v2, VertexNode *v3)
{
    TexCoordNode *t1 = _texCoords.add(v1->data().position);
    TexCoordNode *t2 = _texCoords.add(v2->data().position);
    TexCoordNode *t3 = _texCoords.add(v3->data().position);
    
    VertexNode *vertices[3] = { v1, v2, v3 };
    TexCoordNode *texCoords[3] = { t1, t2, t3 };
    
    _triangles.add(Triangle2(vertices, texCoords));
}

void Mesh2::addQuad(VertexNode *v1, VertexNode *v2, VertexNode *v3, VertexNode *v4)
{
    TexCoordNode *t1 = _texCoords.add(v1->data().position);
    TexCoordNode *t2 = _texCoords.add(v2->data().position);
    TexCoordNode *t3 = _texCoords.add(v3->data().position);
    TexCoordNode *t4 = _texCoords.add(v4->data().position);
    
    VertexNode *vertices1[3] = { v1, v2, v3 };
    VertexNode *vertices2[3] = { v1, v3, v4 };
    
    TexCoordNode *texCoords1[3] = { t1, t2, t3 };
    TexCoordNode *texCoords2[3] = { t1, t3, t4 };
    
  	_triangles.add(Triangle2(vertices1, texCoords1));
    _triangles.add(Triangle2(vertices2, texCoords2));
}

VertexEdgeNode *Mesh2::findOrCreateVertexEdge(VertexNode *v1, VertexNode *v2, TriangleNode *triangle)
{
    VertexEdgeNode *sharedEdge = v1->sharedEdge(v2);
    if (sharedEdge)
    {
        sharedEdge->data().setTriangle(1, triangle);
        return sharedEdge;
    }
    
    VertexNode *vertices[2] = { v1, v2 };
    VertexEdgeNode *node = _vertexEdges.add(vertices);
    node->data().setTriangle(0, triangle);
    return node;
}

TexCoordEdgeNode *Mesh2::findOrCreateTexCoordEdge(TexCoordNode *t1, TexCoordNode *t2, TriangleNode *triangle)
{
    TexCoordEdgeNode *sharedEdge = t1->sharedEdge(t2);
    if (sharedEdge)
    {
        sharedEdge->data().setTriangle(1, triangle);
        return sharedEdge;
    }
    
    TexCoordNode *texCoords[2] = { t1, t2 };
    TexCoordEdgeNode *node = _texCoordEdges.add(texCoords);
    node->data().setTriangle(0, triangle);
    return node;
}

VertexNode *Mesh2::duplicateVertex(VertexNode *original)
{
    if (original->algorithmData.duplicatePair == NULL)
        original->algorithmData.duplicatePair = _vertices.add(original->data().position);
    
    return original->algorithmData.duplicatePair;
}

TexCoordNode *Mesh2::duplicateTexCoord(TexCoordNode *original)
{
    if (original->algorithmData.duplicatePair == NULL)
        original->algorithmData.duplicatePair = _texCoords.add(original->data().position);
    
    return original->algorithmData.duplicatePair;
}

void Mesh2::makeTexCoords()
{
    _texCoords.removeAll();
    
    for (VertexNode *vertex = _vertices.begin(), *end = _vertices.end(); vertex != end; vertex = vertex->next())
    {
        TexCoordNode *texCoord = _texCoords.add(vertex->data().position);
        
        for (SimpleNode<TriangleNode *> *triangle = vertex->_triangles.begin(), 
             *triangleEnd = vertex->_triangles.end(); 
             triangle != triangleEnd; 
             triangle = triangle->next())
        {
            triangle->data()->data().setTexCoordByVertex(texCoord, vertex);
            texCoord->addTriangle(triangle->data());            
        }
    }
}

void Mesh2::makeEdges()
{
    _vertexEdges.removeAll();
    _texCoordEdges.removeAll();
    
    for (VertexNode *node = _vertices.begin(), *end = _vertices.end(); node != end; node = node->next())
    {
        node->removeEdges();
    }
    
    for (TexCoordNode *node = _texCoords.begin(), *end = _texCoords.end(); node != end; node = node->next())
    {
        node->removeEdges();
    }
    
    for (TriangleNode *node = _triangles.begin(), *end = _triangles.end(); node != end; node = node->next())
    {
        Triangle2 &triangle = node->data();
        
        triangle.removeEdges();
        
        VertexNode *v0 = triangle.vertex(0);
        VertexNode *v1 = triangle.vertex(1);
        VertexNode *v2 = triangle.vertex(2);
        
        TexCoordNode *t0 = triangle.texCoord(0);
        TexCoordNode *t1 = triangle.texCoord(1);
        TexCoordNode *t2 = triangle.texCoord(2);
        
        VertexEdgeNode *ve0 = findOrCreateVertexEdge(v0, v1, node);
        VertexEdgeNode *ve1 = findOrCreateVertexEdge(v1, v2, node);
        VertexEdgeNode *ve2 = findOrCreateVertexEdge(v2, v0, node);
        
        triangle.setVertexEdge(0, ve0);
        triangle.setVertexEdge(1, ve1);
        triangle.setVertexEdge(2, ve2);
        
        TexCoordEdgeNode *te0 = findOrCreateTexCoordEdge(t0, t1, node);
        TexCoordEdgeNode *te1 = findOrCreateTexCoordEdge(t1, t2, node);
        TexCoordEdgeNode *te2 = findOrCreateTexCoordEdge(t2, t0, node);

        triangle.setTexCoordEdge(0, te0);
        triangle.setTexCoordEdge(1, te1);
        triangle.setTexCoordEdge(2, te2); 
        
    }
}

void Mesh2::makePlane()
{
    _vertices.removeAll();
    _triangles.removeAll();
    
    VertexNode *v0 = _vertices.add(Vector3D(-1, -1, 0));
	VertexNode *v1 = _vertices.add(Vector3D( 1, -1, 0));
    VertexNode *v2 = _vertices.add(Vector3D( 1,  1, 0));
	VertexNode *v3 = _vertices.add(Vector3D(-1,  1, 0));
    
    addQuad(v0, v1, v2, v3);
    
    makeTexCoords();
    makeEdges();
    
    setSelectionMode(_selectionMode);
}

void Mesh2::makeCube()
{
    _vertices.removeAll();
	_triangles.removeAll();
    _texCoords.removeAll();
    
	// back vertices
	VertexNode *v0 = _vertices.add(Vector3D(-1, -1, -1));
	VertexNode *v1 = _vertices.add(Vector3D( 1, -1, -1));
    VertexNode *v2 = _vertices.add(Vector3D( 1,  1, -1));
	VertexNode *v3 = _vertices.add(Vector3D(-1,  1, -1));
	
	// front vertices
	VertexNode *v4 = _vertices.add(Vector3D(-1, -1,  1));
	VertexNode *v5 = _vertices.add(Vector3D( 1, -1,  1));
	VertexNode *v6 = _vertices.add(Vector3D( 1,  1,  1));
	VertexNode *v7 = _vertices.add(Vector3D(-1,  1,  1));
    
	// back triangles
    addQuad(v0, v1, v2, v3);
	
	// front triangles
    addQuad(v7, v6, v5, v4);
	
	// bottom triangles
    addQuad(v1, v0, v4, v5);
	
	// top triangles
    addQuad(v3, v2, v6, v7);
	
	// left triangles
    addQuad(v7, v4, v0, v3);
	
	// right triangles
    addQuad(v2, v1, v5, v6);
    
    makeTexCoords();
    makeEdges();
	
    setSelectionMode(_selectionMode);
}

void Mesh2::makeCylinder(uint steps)
{
    _vertices.removeAll();
    _triangles.removeAll();
    
    VertexNode *node0 = _vertices.add(Vector3D(0, -1, 0)); // 0
    VertexNode *node1 = _vertices.add(Vector3D(0,  1, 0)); // 1
    
    VertexNode *node2 = _vertices.add(Vector3D(cosf(0.0f), -1, sinf(0.0f))); // 2
    VertexNode *node3 = _vertices.add(Vector3D(cosf(0.0f),  1, sinf(0.0f))); // 3
    
    uint max = steps;
    float step = (FLOAT_PI * 2.0f) / max;
    float angle = step;
    for (uint i = 1; i < max; i++)
    {
        VertexNode *last2 = _vertices.add(Vector3D(cosf(angle), -1, sinf(angle))); // 4
        VertexNode *last1 = _vertices.add(Vector3D(cosf(angle),  1, sinf(angle))); // 5
        
        VertexNode *last3 = last2->previous();
        VertexNode *last4 = last3->previous();
        
        addTriangle(last3, last2, last1);
        addTriangle(last2, last3, last4);
        
        addTriangle(last4, node0, last2);
        addTriangle(last3, last1, node1);
        
        angle += step;
    }
    
    VertexNode *last1 = _vertices.last();
    VertexNode *last2 = last1->previous();
    
    addTriangle(node2, node3, last1);
    addTriangle(last1, last2, node2);
    
    addTriangle(node0, node2, last2);
    addTriangle(node3, node1, last1);
    
    makeTexCoords();
    makeEdges();
    
    setSelectionMode(_selectionMode);
}

void Mesh2::makeSphere(uint steps)
{
    _vertices.removeAll();
    _triangles.removeAll();
    
    uint max = steps;
    
    vector<VertexNode *> tempVertices;
    vector<Triangle> tempTriangles;
    
    tempVertices.push_back(_vertices.add(Vector3D(0, 1, 0)));
    tempVertices.push_back(_vertices.add(Vector3D(0, -1, 0)));
    
    float step = FLOAT_PI / max;
    
    for (uint i = 0; i < max; i++)
    {
        float beta = i * step * 2.0f;
        
        for (uint j = 1; j < max; j++)
        {
            float alpha = 0.5f * FLOAT_PI + j * step;
            float y0 = sinf(alpha);
            float w0 = cosf(alpha);                
            
            float x0 = sinf(beta) * w0;
            float z0 = cosf(beta) * w0;
            
            tempVertices.push_back(_vertices.add(Vector3D(x0, y0, z0)));
            
            if (i > 0 && j < max - 1)
            {
                int index = (i - 1) * (max - 1);
                
                AddQuad(tempTriangles, 
                        1 + max + j + index, 
                        2 + j + index,
                        1 + j + index,
                        max + j + index);                
            }
        }
        
        int index = i * (max - 1);
        if (i < max - 1)
        {
            AddTriangle(tempTriangles,
                        0,
                        2 + index + max - 1,
                        2 + index);
            
            AddTriangle(tempTriangles,
                        1,
                        index + max,
                        index + 2 * max - 1);
        }
        else 
        {
            
            AddTriangle(tempTriangles,
                        0,
                        2,
                        2 + index);
            
            AddTriangle(tempTriangles,
                        1,
                        index + max,
                        max);
        }
    }
    
    for (uint j = 1; j < max - 1; j++)
    {
        int index = (max - 1) * (max - 1);
        AddQuad(tempTriangles,
                1 + j + index,
                1 + j,
                2 + j,
                2 + j + index);
    }
    
    VertexNode *triangleVertices[3];
    
    for (uint i = 0; i < tempTriangles.size(); i++)
    {
        Triangle indexTriangle = tempTriangles[i];
        for (uint j = 0; j < 3; j++)
        {
            VertexNode *node = tempVertices.at(indexTriangle.vertexIndices[j]);
            triangleVertices[j] = node;
        }
        addTriangle(triangleVertices[0], triangleVertices[1], triangleVertices[2]);
    }    
    
    makeTexCoords();
    makeEdges();
    
    setSelectionMode(_selectionMode);
}

void Mesh2::makeIcosahedron()
{
    const float X = 0.525731112119133606f;
    const float Z = 0.850650808352039932f;
    
    static GLfloat vdata[12][3] = 
    {    
        {-X, 0.0, Z}, {X, 0.0, Z}, {-X, 0.0, -Z}, {X, 0.0, -Z},    
        {0.0, Z, X}, {0.0, Z, -X}, {0.0, -Z, X}, {0.0, -Z, -X},    
        {Z, X, 0.0}, {-Z, X, 0.0}, {Z, -X, 0.0}, {-Z, -X, 0.0} 
    };
    
    static GLint tindices[20][3] = 
    { 
        {0,4,1}, {0,9,4}, {9,5,4}, {4,5,8}, {4,8,1},    
        {8,10,1}, {8,3,10}, {5,3,8}, {5,2,3}, {2,7,3},    
        {7,10,3}, {7,6,10}, {7,11,6}, {11,0,6}, {0,1,6}, 
        {6,1,10}, {9,0,11}, {9,11,2}, {9,2,5}, {7,2,11} 
    };
    
    vector<VertexNode *> vertexNodes;
    
    for (int i = 0; i < 12; i++)
    {
        vertexNodes.push_back(_vertices.add(Vector3D(vdata[i])));
    }
    
    for (int i = 0; i < 20; i++)
    {
        VertexNode *triangleVertices[3];
        for (int j = 0; j < 3; j++)
            triangleVertices[j] = vertexNodes[tindices[i][j]];
        _triangles.add(Triangle2(triangleVertices));
    }
    
    makeTexCoords();
    makeEdges();
    
    setSelectionMode(_selectionMode);
}

void Mesh2::fromVertices(const vector<Vector3D> &vertices)
{
    resetTriangleCache();
    _vertices.removeAll();
    _texCoords.removeAll();
    _triangles.removeAll();
    
    vector<VertexNode *> tempVertices;
    vector<VertexNode *> uniqueVertices;
    
    uint verticesSize = vertices.size();
    
    for (uint i = 0; i < verticesSize; i++)
    {
        bool found = false;
        
        for (uint j = 0; j < uniqueVertices.size(); j++)
        {
            if (uniqueVertices[j]->data().position.SqDistance(vertices[i]) < FLOAT_EPS)
            {
                VertexNode *last = tempVertices.size() > 0 ? tempVertices[tempVertices.size() - 1] : NULL;
                VertexNode *lastPrevious = tempVertices.size() > 1 ? tempVertices[tempVertices.size() - 2] : NULL;
                
                if (last == uniqueVertices[j] || lastPrevious == uniqueVertices[j])
                    break;
                
                tempVertices.push_back(uniqueVertices[j]);
                found = true;
                break;
            }            
        }
        
        if (!found)
        {
            VertexNode *newVertex = _vertices.add(vertices[i]);
            uniqueVertices.push_back(newVertex);
            tempVertices.push_back(newVertex);
        }        
    }
    
    VertexNode *triangleVertices[3];
    
    for (uint i = 0; i < verticesSize; i += 3)
    {
        for (uint j = 0; j < 3; j++)
        {
            triangleVertices[j] = tempVertices.at(i + j);
        }        
        _triangles.add(Triangle2(triangleVertices));
    }    
    
    makeTexCoords();
    makeEdges();
    
    setSelectionMode(_selectionMode);
}

void Mesh2::toVertices(vector<Vector3D> &vertices)
{
    for (TriangleNode *node = _triangles.begin(), *end = _triangles.end(); node != end; node = node->next())
    {
        for (int j = 0; j < 3; j++)
        {
            Vector3D vertex = node->data().vertex(j)->data().position;
            vertices.push_back(vertex);
        }        
    }
}

void Mesh2::fromIndexRepresentation(const vector<Vector3D> &vertices, const vector<Vector3D> &texCoords, const vector<Triangle> &triangles)
{
    resetTriangleCache();
    _vertices.removeAll();
    _texCoords.removeAll();
    _triangles.removeAll();
    
    vector<VertexNode *> tempVertices;
    vector<TexCoordNode *> tempTexCoords;
    
    for (uint i = 0; i < vertices.size(); i++)
    {
        tempVertices.push_back(_vertices.add(vertices[i]));
    }
    
    for (uint i = 0; i < texCoords.size(); i++)
    {
        tempTexCoords.push_back(_texCoords.add(texCoords[i]));
    }
    
    VertexNode *triangleVertices[3];
    TexCoordNode *triangleTexCoords[3];
    
    for (uint i = 0; i < triangles.size(); i++)
    {
        const Triangle &indexTriangle = triangles[i];
        for (uint j = 0; j < 3; j++)
        {
            triangleVertices[j] = tempVertices.at(indexTriangle.vertexIndices[j]);
            triangleTexCoords[j] = tempTexCoords.at(indexTriangle.texCoordIndices[j]);
        }        
        _triangles.add(Triangle2(triangleVertices, triangleTexCoords));
    }    
    
    makeEdges();
    
    setSelectionMode(_selectionMode);
}

void Mesh2::toIndexRepresentation(vector<Vector3D> &vertices, vector<Vector3D> &texCoords, vector<Triangle> &triangles)
{
    int index = 0;
    
    for (VertexNode *node = _vertices.begin(), *end = _vertices.end(); node != end; node = node->next())
    {
        node->algorithmData.index = index;
        index++;
        
        vertices.push_back(node->data().position);
    }
    
    index = 0;
    
    for (TexCoordNode *node = _texCoords.begin(), *end = _texCoords.end(); node != end; node = node->next())
    {
        node->algorithmData.index = index;
        index++;
        
        texCoords.push_back(node->data().position);
    }
    
    for (TriangleNode *node = _triangles.begin(), *end = _triangles.end(); node != end; node = node->next())
    {
        Triangle indexTriangle;
        for (int j = 0; j < 3; j++)
        {
            indexTriangle.vertexIndices[j] = node->data().vertex(j)->algorithmData.index;
            indexTriangle.texCoordIndices[j] = node->data().texCoord(j)->algorithmData.index;
        }        
        triangles.push_back(indexTriangle);
    }
}

void Mesh2::setSelection(const vector<bool> &selection)
{
    for (uint i = 0; i < selection.size(); i++)
    {
        if (!selection[i])
            setSelectedAtIndex(selection[i], i);
    }
    
    for (uint i = 0; i < selection.size(); i++)
    {
        if (selection[i])
            setSelectedAtIndex(selection[i], i);
    }
}

void Mesh2::getSelection(vector<bool> &selection)
{
    selection.clear();
    for (uint i = 0; i < selectedCount(); i++)
        selection.push_back(isSelectedAtIndex(i));
}

