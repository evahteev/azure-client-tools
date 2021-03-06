// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "stdafx.h"
#include "PluginJsonConstants.h"
#include "..\..\AzureDeviceManagementCommon\DMConstants.h"
#include "..\..\AzureDeviceManagementCommon\Plugins\PluginConstants.h"
#include "SkeletonStateHandler.h"

using namespace DMCommon;
using namespace DMUtils;
using namespace std;

constexpr char InterfaceVersion[] = "1.0.0";

namespace Microsoft { namespace Azure { namespace DeviceManagement { namespace SkeletonPlugin {

    SkeletonStateHandler::SkeletonStateHandler() :
        BaseHandler(SkeletonStateHandlerId, ReportedSchema(JsonDeviceSchemasTypeRaw, JsonDeviceSchemasTagDM, InterfaceVersion))
    {
    }

    void SkeletonStateHandler::Start(
        const Json::Value& handlerConfig,
        bool& active)
    {
        TRACELINE(LoggingLevel::Verbose, __FUNCTION__);

        SetConfig(handlerConfig);

        Json::Value logFilesPath = handlerConfig[JsonTextLogFilesPath];
        if (!logFilesPath.isNull() && logFilesPath.isString())
        {
            wstring wideLogFileName = MultibyteToWide(logFilesPath.asString().c_str());
            wstring wideLogFileNamePrefix = MultibyteToWide(GetId().c_str());
            gLogger.SetLogFilePath(wideLogFileName.c_str(), wideLogFileNamePrefix.c_str());
            gLogger.EnableConsole(true);

            TRACELINE(LoggingLevel::Verbose, "Logging configured.");
        }

        active = true;
    }

    void SkeletonStateHandler::OnConnectionStatusChanged(
        DMCommon::ConnectionStatus status)
    {
        TRACELINE(LoggingLevel::Verbose, __FUNCTION__);
        if (status == ConnectionStatus::eOffline)
        {
            TRACELINE(LoggingLevel::Verbose, "Connection Status: Offline.");
        }
        else
        {
            TRACELINE(LoggingLevel::Verbose, "Connection Status: Online.");

            Json::Value handlerConfig = GetConfig();

            // Report when connected to cloud and Json handler is set "reportOnConnect": true....
            Json::Value reportOnConnect = handlerConfig[JsonHandlerConfigReportOnConnect];
            if (!reportOnConnect.isNull() && reportOnConnect.isBool())
            {
                if (reportOnConnect.asBool())
                {
                    TRACELINE(LoggingLevel::Verbose, "Report on connect is on...");

                    Json::Value reportedObject(Json::objectValue);
                    std::shared_ptr<ReportedErrorList> errorList = make_shared<ReportedErrorList>();
                    FinalizeAndReport(reportedObject, errorList);
                }
                else
                {
                    TRACELINE(LoggingLevel::Verbose, "Report on connect is off...");
                }
            }
        }
    }

    void SkeletonStateHandler::BuildReported(
        Json::Value& reportedObject,
        std::shared_ptr<DMCommon::ReportedErrorList> errorList)
    {
        (void)reportedObject;
        (void)errorList;

        // reportedObject["propetyName"] = propertyValue;
    }

    void SkeletonStateHandler::EmptyReported(
        Json::Value& reportedObject)
    {
        (void)reportedObject;

        Json::Value nullValue;

        // reportedObject["propetyName"] = nullValue;
    }

    InvokeResult SkeletonStateHandler::Invoke(
        const Json::Value& groupDesiredConfigJson) noexcept
    {
        TRACELINE(LoggingLevel::Verbose, __FUNCTION__);

        // Returned objects (if InvokeContext::eDesiredState, no return present for invokeResult).
        InvokeResult invokeResult(InvokeContext::eDesiredState);

        // Twin reported objects
        Json::Value reportedObject(Json::objectValue);
        std::shared_ptr<ReportedErrorList> errorList = make_shared<ReportedErrorList>();

        RunOperation(GetId(), errorList,
            [&]()
        {
            // Make sure this is not a transient state
            if (IsRefreshing(groupDesiredConfigJson))
            {
                return;
            }

            // Processing Meta Data
            _metaData->FromJsonParentObject(groupDesiredConfigJson);
            string serviceInterfaceVersion = _metaData->GetServiceInterfaceVersion();

            // Report refreshing
            ReportRefreshing();

            //Compare interface version with the interface version sent by service
            if (MajorVersionCompare(InterfaceVersion, serviceInterfaceVersion) == 0)
            {
                // Apply sub-group 0
                // Apply sub-group 1
                // etc

                // Report current state
                if (_metaData->GetReportingMode() == JsonReportingModeDefault)
                {
                    BuildReported(reportedObject, errorList);
                }
                else
                {
                    EmptyReported(reportedObject);
                }
                _metaData->SetOutputInterfaceVersion(InterfaceVersion);
            }
            else
            {
                throw DMException(DMSubsystem::DeviceAgentPlugin, DM_PLUGIN_ERROR_INVALID_INTERFACE_VERSION, "Service solution is trying to talk with Interface Version that is not supported.");
            }
        });

        // Update device twin
        FinalizeAndReport(reportedObject, errorList);

        // Pack return payload
        if (errorList->Count() != 0)
        {
            invokeResult.code = JsonDirectMethodFailureCode;
            invokeResult.payload = errorList->ToJsonObject()[GetId()].toStyledString();
        }
        return invokeResult;
    }

}}}}