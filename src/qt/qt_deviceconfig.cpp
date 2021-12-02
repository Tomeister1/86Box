#include "qt_deviceconfig.hpp"
#include "ui_qt_deviceconfig.h"

#include <QDebug>
#include <QComboBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QCheckBox>

extern "C" {
#include <86box/86box.h>
#include <86box/config.h>
#include <86box/device.h>
}

#include "qt_filefield.hpp"

DeviceConfig::DeviceConfig(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DeviceConfig)
{
    ui->setupUi(this);
}

DeviceConfig::~DeviceConfig()
{
    delete ui;
}

void DeviceConfig::ConfigureDevice(const _device_* device) {
    DeviceConfig dc;
    dc.setWindowTitle(QString("%1 Device Configuration").arg(device->name));

    device_context_t device_context;
    device_set_context(&device_context, device, 0);

    const auto* config = device->config;
    while (config->type != -1) {
        switch (config->type) {
        case CONFIG_BINARY:
        {
            auto value = config_get_int(device_context.name, const_cast<char*>(config->name), config->default_int);
            auto* cbox = new QCheckBox();
            cbox->setObjectName(config->name);
            cbox->setChecked(value > 0);
            dc.ui->formLayout->addRow(config->description, cbox);
            break;
        }
        case CONFIG_SELECTION:
        case CONFIG_MIDI:
        case CONFIG_MIDI_IN:
        case CONFIG_HEX16:
        case CONFIG_HEX20:
        {
            auto* cbox = new QComboBox();
            cbox->setObjectName(config->name);
            auto* model = cbox->model();
            int currentIndex = -1;
            int selected = config_get_int(device_context.name, const_cast<char*>(config->name), config->default_int);

            for (auto* sel = config->selection; (sel->description != nullptr) && (strlen(sel->description) > 0); ++sel) {
                int rows = model->rowCount();
                model->insertRow(rows);
                auto idx = model->index(rows, 0);

                model->setData(idx, sel->description, Qt::DisplayRole);
                model->setData(idx, sel->value, Qt::UserRole);

                if (selected == sel->value) {
                    currentIndex = idx.row();
                }
            }

            dc.ui->formLayout->addRow(config->description, cbox);
            cbox->setCurrentIndex(currentIndex);
            break;
        }
        case CONFIG_SPINNER:
        {
            int value = config_get_int(device_context.name, const_cast<char*>(config->name), config->default_int);
            auto* spinBox = new QSpinBox();
            spinBox->setObjectName(config->name);
            spinBox->setMaximum(config->spinner.max);
            spinBox->setMinimum(config->spinner.min);
            if (config->spinner.step > 0) {
                spinBox->setSingleStep(config->spinner.step);
            }
            spinBox->setValue(value);
            dc.ui->formLayout->addRow(config->description, spinBox);
            break;
        }
        case CONFIG_FNAME:
        {
            auto* fileName = config_get_string(device_context.name, const_cast<char*>(config->name), const_cast<char*>(config->default_string));
            auto* fileField = new FileField();
            fileField->setObjectName(config->name);
            fileField->setFileName(fileName);
            dc.ui->formLayout->addRow(config->description, fileField);
            break;
        }
        }
        ++config;
    }

    int res = dc.exec();
    if (res == QDialog::Accepted) {
        config = device->config;
        while (config->type != -1) {
            switch (config->type) {
            case CONFIG_BINARY:
            {
                auto* cbox = dc.findChild<QCheckBox*>(config->name);
                config_set_int(device_context.name, const_cast<char*>(config->name), cbox->isChecked() ? 1 : 0);
                break;
            }
            case CONFIG_SELECTION:
            case CONFIG_MIDI:
            case CONFIG_MIDI_IN:
            case CONFIG_HEX16:
            case CONFIG_HEX20:
            {
                auto* cbox = dc.findChild<QComboBox*>(config->name);
                config_set_int(device_context.name, const_cast<char*>(config->name), cbox->currentData().toInt());
                break;
            }
            case CONFIG_FNAME:
            {
                auto* fbox = dc.findChild<FileField*>(config->name);
                auto fileName = fbox->fileName().toUtf8();
                config_set_string(device_context.name, const_cast<char*>(config->name), fileName.data());
                break;
            }
            case CONFIG_SPINNER:
            {
                auto* spinBox = dc.findChild<QSpinBox*>(config->name);
                config_set_int(device_context.name, const_cast<char*>(config->name), spinBox->value());
                break;
            }
            }
            config++;
        }
    }
}

QString DeviceConfig::DeviceName(const _device_* device, const char *internalName, int bus) {
    if (QStringLiteral("none") == internalName) {
        return "None";
    } else if (QStringLiteral("internal") == internalName) {
        return "Internal";
    } else if (device == nullptr) {
        return QString();
    } else {
        char temp[512];
        device_get_name(device, bus, temp);
        return temp;
    }
}
