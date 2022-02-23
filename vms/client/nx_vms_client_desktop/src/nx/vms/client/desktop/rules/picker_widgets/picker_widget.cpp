// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "picker_widget.h"

namespace nx::vms::client::desktop::rules {

using Field = vms::rules::Field;
using FieldDescriptor = vms::rules::FieldDescriptor;

PickerWidget::PickerWidget(QWidget* parent):
    QWidget(parent)
{
}

void PickerWidget::setDescriptor(const FieldDescriptor& descriptor)
{
    fieldDescriptor = descriptor;

    onDescriptorSet();
}

std::optional<FieldDescriptor> PickerWidget::descriptor() const
{
    return fieldDescriptor;
}

bool PickerWidget::hasDescriptor() const
{
    return fieldDescriptor != std::nullopt;
}

void PickerWidget::setField(Field* value)
{
}

void PickerWidget::onFieldSet()
{
}

} // namespace nx::vms::client::desktop::rules