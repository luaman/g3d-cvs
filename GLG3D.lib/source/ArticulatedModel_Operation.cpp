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
void ArticulatedModel::TriListOperation::parseTarget(const Any& any) {
    if (any.size() == 3) {
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

    } else if (any.size() == 2) {
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

    } else if (any.size() == 1) {
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
    op->parseTarget(any);
    return op;
}


void ArticulatedModel::RemoveOperation::process(ArticulatedModel::Ref model, int partIndex, Part& part) {
    if ((sourceTriList.size() == 1) && (sourceTriList[0] == ALL)) {
        part.removeGeometry();
    } else {
        // Remove from back to front so that ordering is not affected while removing
        sourceTriList.sort(SORT_DECREASING);
        for (int i = 0; i < sourceTriList.size(); ++i) {
            part.triList.remove(sourceTriList[i]);
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
    op->parseTarget(any);
    op->twoSided = any.last();
    return op;
}


void ArticulatedModel::SetTwoSidedOperation::process(ArticulatedModel::Ref model, int partIndex, Part& part) {
    if ((sourceTriList.size() == 1) && (sourceTriList[0] == ALL)) {
        for (int t = 0; t < part.triList.size(); ++t) {
            part.triList[t]->twoSided = twoSided;
        }
    } else {
        for (int t = 0; t < sourceTriList.size(); ++t) {
            part.triList[sourceTriList[t]]->twoSided = twoSided;
        }
    }
}

////////////////////////////////////////////////////////////////////////

ArticulatedModel::SetMaterialOperation::Ref ArticulatedModel::SetMaterialOperation::create(const Any& any) {
    any.verifyName("setMaterial");
    any.verify(any.size() <= 3, "Cannot take more than three arguments");

    Ref op = new SetMaterialOperation();
    op->parseTarget(any);
    op->material = Material::create(any.last());
    return op;
}


void ArticulatedModel::SetMaterialOperation::process(ArticulatedModel::Ref model, int partIndex, Part& part) {
    if ((sourceTriList.size() == 1) && (sourceTriList[0] == ALL)) {
        for (int t = 0; t < part.triList.size(); ++t) {
            part.triList[t]->material = material;
        }
    } else {
        for (int t = 0; t < sourceTriList.size(); ++t) {
            part.triList[sourceTriList[t]]->material = material;
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
        normal[i] = (nform * Vector4(vertex[i], 0.0)).xyz().direction();
    }
}

} // namespace G3D
