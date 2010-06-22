/** @file ArticulatedModel.h
    @author Morgan McGuire, http://graphics.cs.williams.edu
*/
#include "G3D/Any.h"
#include "GLG3D/ArticulatedModel.h"

namespace G3D {

ArticulatedModel::PartID::PartID(const std::string& name) : m_index(USE_NAME), m_name(name) {
}


ArticulatedModel::PartID::PartID(int index) : m_index(index) {
    debugAssert(index == ALL || index >= 0);
}


ArticulatedModel::PartID::PartID(const Any& any) : m_index(USE_NAME){
    any.verifyType(Any::STRING, Any::NUMBER);
    if (any.type() == Any::STRING) {
        m_name = any.string();
    } else {
        m_index = int(any.number());
        any.verify(m_index >= 0, "Part index must be non-negative.");
    }
}

///////////////////////////////////////////////


ArticulatedModel::Operation::Ref ArticulatedModel::Operation::create(const Any& any) {
    any.verifyType(Any::ARRAY);
    if (any.nameEquals("rename")) {
        return RenameOperation::create(any);
    } else if (any.nameEquals("remove")) {
        return RemoveOperation::create(any);
    } else if (any.nameEquals("setTwoSided")) {
        return SetTwoSidedOperation::create(any);
    } else if (any.nameEquals("setMaterial")) {
        return SetMaterialOperation::create(any);
    } else if (any.nameEquals("transform")) {
        return TransformOperation::create(any);
    } else if (any.nameEquals("merge")) {
        return MergeOperation::create(any);
    } else if (any.nameEquals("setCFrame")) {
        return SetCFrameOperation::create(any);
    } else if (any.nameEquals("mergeByMaterial")) {
        return MergeByMaterialOperation::create(any);
    } else {
        any.verify(false, "Unrecognized operation type: " + any.name());
        return NULL;
    }
}

////////////////////////////////////////////////////////////////////////

ArticulatedModel::RenameOperation::Ref ArticulatedModel::RenameOperation::create(const Any& any) {
    any.verifyName("rename");
    any.verifySize(2);

    Ref op = new RenameOperation();
    op->sourcePart = any[0];
    op->name = any[1].string();

    return op;
}
        

void ArticulatedModel::RenameOperation::apply(ArticulatedModel::Ref model) {
    model->partArray[model->partIndex(sourcePart)].name = name;
}

////////////////////////////////////////////////////////////////////////

ArticulatedModel::SetCFrameOperation::Ref ArticulatedModel::SetCFrameOperation::create(const Any& any) {
    any.verifyName("setCFrame");
    any.verifySize(2);

    Ref op = new SetCFrameOperation();

    Any parts = any[0];
    if (parts.type() == Any::ARRAY) {
        op->sourcePart.resize(parts.size());
        for (int i = 0; i < parts.size(); ++i) {            
            op->sourcePart[i] = parts[i];
        }
    } else {
        op->sourcePart.append(parts);
    }
    op->cframe = any[1];

    return op;
}
        

void ArticulatedModel::SetCFrameOperation::apply(ArticulatedModel::Ref model) {
    for (int i = 0; i < sourcePart.size(); ++i) {
        Part& p = model->partArray[model->partIndex(sourcePart[i])];
        p.cframe = cframe;
    }
}

////////////////////////////////////////////////////////////////////////
void ArticulatedModel::TriListOperation::parseTarget(const Any& any, int numExtraArgs) {
    int myNumArgs = any.size() - numExtraArgs;
    if (myNumArgs == 2) {
        // part, (trilist) or part, trilist
        sourcePart.append(any[0]);

        Any t = any[1];
        if (t.type() == Any::ARRAY) {
            // List of trilists
            sourceTriList.resize(t.size());
            for (int i = 0; i < sourceTriList.size(); ++i) {
                sourceTriList[i] = t[i];
                t[i].verify(sourceTriList[i] >= 0, "triList index must be non-negative");
            }
        } else {
            // Single trilist
            sourceTriList.append(t.number());
            t.verify(sourceTriList[0] >= 0, "triList index must be non-negative");
        }

    } else if (myNumArgs == 1) {
        // part or (part)
        Any p = any[0];
        if (p.type() == Any::ARRAY) {
            sourcePart.resize(p.size());
            for (int i = 0; i < sourcePart.size(); ++i) {
                sourcePart[i] = p[i];
            }
        } else {
            sourcePart.append(p);
        }
        sourceTriList.append(ALL);

    } else if (myNumArgs == 0) {
        // all parts
        sourcePart.append(ALL);
        sourceTriList.append(ALL);
    }
}


void ArticulatedModel::TriListOperation::apply(ArticulatedModel::Ref model) {
    if (sourcePart.size() == 1 && sourcePart[0].isAll()) {
        for (int i = 0; i < model->partArray.size(); ++i) {
            Part& p = model->partArray[i];
            process(model, i, p);
        }
    } else {
        for (int i = 0; i < sourcePart.size(); ++i) {
            Part& p = model->partArray[model->partIndex(sourcePart[i])];
            process(model, i, p);
        }
    }
}

////////////////////////////////////////////////////////////////////////

ArticulatedModel::RemoveOperation::Ref ArticulatedModel::RemoveOperation::create(const Any& any) {
    any.verifyName("remove");

    Ref op = new RemoveOperation();
    op->parseTarget(any, 0);
    return op;
}


void ArticulatedModel::RemoveOperation::process(ArticulatedModel::Ref model, int partIndex, Part& part) {
    if ((sourceTriList.size() == 1) && (sourceTriList[0] == ALL)) {
        part.removeGeometry();
    } else {
        for (int i = 0; i < sourceTriList.size(); ++i) {
            part.triList[sourceTriList[i]] = NULL;
        }

        // Compact
        for (int i = part.triList.size() - 1; i >= -1; --i) {
            if ((i == -1) || part.triList[i].notNull()) {
                // This is the last non-null
                part.triList.resize(i + 1);
                break;
            }
        }

        if (part.triList.size() == 0) {
            part.removeGeometry();
        }
    }
}


////////////////////////////////////////////////////////////////////////

ArticulatedModel::SetTwoSidedOperation::Ref ArticulatedModel::SetTwoSidedOperation::create(const Any& any) {
    any.verifyName("setTwoSided");
    any.verify(any.size() <= 3, "Cannot take more than three arguments");

    Ref op = new SetTwoSidedOperation();
    op->parseTarget(any, 1);
    op->twoSided = any.last();
    return op;
}


void ArticulatedModel::SetTwoSidedOperation::process(ArticulatedModel::Ref model, int partIndex, Part& part) {
    if ((sourceTriList.size() == 1) && (sourceTriList[0] == ALL)) {
        for (int t = 0; t < part.triList.size(); ++t) {
            if (part.triList[t].notNull()) {
                part.triList[t]->twoSided = twoSided;
            }
        }
    } else {
        for (int t = 0; t < sourceTriList.size(); ++t) {
            Part::TriList::Ref tri = part.triList[sourceTriList[t]];
            if (tri.notNull()) {
                tri->twoSided = twoSided;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////

ArticulatedModel::SetMaterialOperation::Ref ArticulatedModel::SetMaterialOperation::create(const Any& any) {
    any.verifyName("setMaterial");
    any.verify(any.size() <= 3, "Cannot take more than three arguments");

    Ref op = new SetMaterialOperation();
    op->parseTarget(any, 1);
    op->material = Material::create(any.last());
    return op;
}


void ArticulatedModel::SetMaterialOperation::process(ArticulatedModel::Ref model, int partIndex, Part& part) {
    if ((sourceTriList.size() == 1) && (sourceTriList[0] == ALL)) {
        for (int t = 0; t < part.triList.size(); ++t) {
            if (part.triList[t].notNull()) {
                part.triList[t]->material = material;
            }
        }
    } else {
        for (int t = 0; t < sourceTriList.size(); ++t) {
            Part::TriList::Ref tri = part.triList[sourceTriList[t]];
            if (tri.notNull()) {
                tri->material = material;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////

ArticulatedModel::TransformOperation::Ref ArticulatedModel::TransformOperation::create(const Any& any) {
    any.verifyName("transform");

    Ref op = new TransformOperation();

    if (any.size() == 2) {
        // part or (part)
        Any p = any[0];
        if (p.type() == Any::ARRAY) {
            op->sourcePart.resize(p.size());
            for (int i = 0; i < op->sourcePart.size(); ++i) {
                op->sourcePart[i] = p[i];
            }
        } else {
            op->sourcePart.append(p);
        }
    } else {
        // all parts
        op->sourcePart.append(ALL);
    }

    op->xform = any.last();

    return op;
}
        

void ArticulatedModel::TransformOperation::apply(ArticulatedModel::Ref model) {
    if (sourcePart.size() == 1 && sourcePart[0].isAll()) {
        for (int i = 0; i < model->partArray.size(); ++i) {
            transform(model->partArray[i]);
        }
    } else {
        for (int i = 0; i < sourcePart.size(); ++i) {
            transform(model->partArray[model->partIndex(sourcePart[i])]);
        }
    }
}


void ArticulatedModel::TransformOperation::transform(ArticulatedModel::Part& part) const {
    Array<Vector3>& vertex = part.geometry.vertexArray;
    Array<Vector3>& normal = part.geometry.normalArray;

    Matrix4 nform = xform.inverse().transpose();
    for (int i = 0; i < vertex.size(); ++i) {
        vertex[i] = (xform * Vector4(vertex[i], 1.0)).xyz();
    }
    for (int i = 0; i < normal.size(); ++i) {
        normal[i] = (nform * Vector4(vertex[i], 0.0)).xyz().direction();
    }
}

////////////////////////////////////////////////////////////////////////

ArticulatedModel::MergeOperation::Ref ArticulatedModel::MergeOperation::create(const Any& any) {
    any.verifyName("merge");

    Ref op = new MergeOperation();
    op->part.resize(any.size());
    for (int i = 0; i < op->part.size(); ++i) {
        op->part[i] = any[i];
    }

    return op;
}
        

void ArticulatedModel::MergeOperation::apply(ArticulatedModel::Ref model) {
    // The target part
    Part& targetPart = model->partArray[model->partIndex(part[0])];

    alwaysAssertM(targetPart.triList.size() > 0, "No trilists in target part");

    Part::TriList::Ref targetTriList = targetPart.triList[0];

    debugAssert(targetTriList.notNull());

    Array<int>& targetIndexArray = targetTriList->indexArray;
    // Merge all tri-lists within the target part
    for (int t = 1; t < targetPart.triList.size(); ++t) {
        if (targetPart.triList[t].notNull()) {
            targetIndexArray.append(targetPart.triList[t]->indexArray);
        }
    }

    // Erase the excesss trilists, which are now unused
    targetPart.triList.resize(1);

    const bool needTexCoords = targetPart.texCoordArray.size() == targetPart.geometry.vertexArray.size();

    // Merge all parts 
    for (int p = 1; p < part.size(); ++p) {
        const int offset = targetPart.geometry.vertexArray.size();
        Part& sourcePart = model->partArray[model->partIndex(part[p])];
        
        // TODO: (maybe we should apply their transformations)

        targetPart.geometry.vertexArray.append(sourcePart.geometry.vertexArray);
        targetPart.geometry.normalArray.append(sourcePart.geometry.normalArray);

        // Append the arrays
        if (needTexCoords) {
            // We need texture coordinates
            if (sourcePart.texCoordArray.size() == 0) {
                // There are no texture coordinates on the source part, so make some
                targetPart.texCoordArray.resize(targetPart.geometry.vertexArray.size());
            } else {
                // Copy the tex coords
                targetPart.texCoordArray.append(sourcePart.texCoordArray);
            }
        }

        debugAssert(targetPart.texCoordArray.size() == 0 ||
            targetPart.texCoordArray.size() == targetPart.geometry.vertexArray.size());

        // Offset the indices and add them
        for (int t = 0; t < sourcePart.triList.size(); ++t) {
            Part::TriList::Ref sourceTriList = sourcePart.triList[t];
            const Array<int>& sourceIndexArray = sourceTriList->indexArray;
            for (int i = 0; i < sourceIndexArray.size(); ++i) {
                targetIndexArray.append(sourceIndexArray[i] + offset);
            }
        }

        // Erase the contents of the part
        sourcePart.removeGeometry();
    }
}


////////////////////////////////////////////////////////////////////////

ArticulatedModel::MergeByMaterialOperation::Ref ArticulatedModel::MergeByMaterialOperation::create(const Any& any) {
    any.verifyName("mergeByMaterial");
    any.verifySize(0);

    Ref op = new MergeByMaterialOperation();
    
    return op;
}
        

void ArticulatedModel::MergeByMaterialOperation::apply(ArticulatedModel::Ref model) {
    // For each destination
    for (int dp = 0; dp < model->partArray.size(); ++dp) {
        Part& destPart = model->partArray[dp];
        for (int dt = 0; dt < destPart.triList.size(); ++dt) {
            Part::TriList::Ref destTriList = destPart.triList[dt];

            if (destTriList.notNull()) {
                // For each source
                for (int sp = dp; sp < model->partArray.size(); ++sp) {
                    Part& sourcePart = model->partArray[sp];

                    int numNull = 0;
                    int startIndex = 0;
                    bool samePart = (sp == dp);

                    if (samePart) {
                        // We're in the same part, so don't look at earlier triLists
                        startIndex = dt + 1;
                    }

                    bool copyGeom = ! samePart;
                    int indexOffset = 0;

                    for (int st = startIndex; st < sourcePart.triList.size(); ++st) {
                        Part::TriList::Ref sourceTriList = sourcePart.triList[st];

                        if (sourceTriList.notNull()) {
                            if ((*(sourceTriList->material) == *(destTriList->material)) && (sourceTriList->primitive == destTriList->primitive)) {
                                // Merge into dest
                                if (samePart) {
                                    // We already share the geometry, so just append the indices
                                    destTriList->indexArray.append(sourceTriList->indexArray);

                                } else {

                                    if (copyGeom) {
                                        const CFrame& xform = destPart.cframe.inverse() * sourcePart.cframe;

                                        // Append arrays
                                        indexOffset = destPart.geometry.vertexArray.size();

                                        // TODO: Don't assume that parts are not nested
                                        alwaysAssertM(sourcePart.parent == -1 && destPart.parent == -1, "child part merging is not implemented in this release");
                                        destPart.geometry.vertexArray.resize(sourcePart.geometry.vertexArray.size() + indexOffset);
                                        destPart.geometry.normalArray.resize(sourcePart.geometry.normalArray.size() + indexOffset);
                                        for (int i = 0; i < sourcePart.geometry.vertexArray.size(); ++i) {
                                            destPart.geometry.vertexArray[i + indexOffset] = xform.pointToWorldSpace(sourcePart.geometry.vertexArray[i]);
                                        }
                                        for (int i = 0; i < sourcePart.geometry.normalArray.size(); ++i) {
                                            destPart.geometry.normalArray[i + indexOffset] = xform.normalToWorldSpace(sourcePart.geometry.normalArray[i]);
                                        }

                                        // Only copy tex coords if dest expects them or if dest has no tex coords already
                                        if ((destPart.texCoordArray.size() != 0) || (indexOffset == 0)) {
                                            destPart.texCoordArray.append(sourcePart.texCoordArray);
                                        }
                                        copyGeom = false;
                                    }

                                    // Append indices, renumbering
                                    const int renumBegin = destTriList->indexArray.size();
                                    destTriList->indexArray.append(sourceTriList->indexArray);
                                    Array<int>& index = destTriList->indexArray;
                                    for (int i = renumBegin; i < index.size(); ++i) {
                                        index[i] += indexOffset;
                                    }
                                }

                                // Remove from source
                                sourcePart.triList[st] = NULL;
                                ++numNull;
                            }
                        } else {
                            ++numNull;
                        }
                    }

                    if (sourcePart.triList.size() == numNull) {
                        // There's nothing left in this part
                        sourcePart.removeGeometry();
                    }
                }
            }
        }
    }
    // TODO
}

} // namespace G3D
