/******************************************************************************
    Copyright (C) 2026

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
******************************************************************************/

#include "PresentationImporter.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

#include <algorithm>
#include <cmath>
#include <future>
#include <limits>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#include <OleAuto.h>
#include <wrl/client.h>

#define _SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Data.Pdf.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Storage.h>
#include <winrt/base.h>

using Microsoft::WRL::ComPtr;

namespace {
QString HResultMessage(HRESULT result)
{
	wchar_t *message = nullptr;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		       nullptr, result, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		       reinterpret_cast<wchar_t *>(&message), 0, nullptr);
	const QString text = message ? QString::fromWCharArray(message).trimmed() : QString();
	if (message)
		LocalFree(message);
	return text;
}

VARIANT StringVariant(const QString &value)
{
	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_BSTR;
	variant.bstrVal = SysAllocString(reinterpret_cast<const OLECHAR *>(value.utf16()));
	return variant;
}

VARIANT BoolVariant(bool value)
{
	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_BOOL;
	variant.boolVal = value ? VARIANT_TRUE : VARIANT_FALSE;
	return variant;
}

VARIANT IntVariant(int value)
{
	VARIANT variant;
	VariantInit(&variant);
	variant.vt = VT_I4;
	variant.lVal = value;
	return variant;
}

bool Invoke(IDispatch *object, const wchar_t *name, WORD flags, std::vector<VARIANT> &arguments,
	    VARIANT *result, QString &error)
{
	DISPID id = 0;
	LPOLESTR mutableName = const_cast<LPOLESTR>(name);
	HRESULT hr = object->GetIDsOfNames(IID_NULL, &mutableName, 1, LOCALE_USER_DEFAULT, &id);
	if (FAILED(hr)) {
		error = QStringLiteral("%1: %2").arg(QString::fromWCharArray(name), HResultMessage(hr));
		return false;
	}
	std::vector<VARIANTARG> reversed(arguments.size());
	for (size_t index = 0; index < arguments.size(); ++index) {
		VariantInit(&reversed[index]);
		VariantCopy(&reversed[index], &arguments[arguments.size() - index - 1]);
	}
	DISPPARAMS params{};
	params.rgvarg = reversed.empty() ? nullptr : reversed.data();
	params.cArgs = static_cast<UINT>(reversed.size());
	DISPID namedId = DISPID_PROPERTYPUT;
	if (flags & DISPATCH_PROPERTYPUT) {
		params.rgdispidNamedArgs = &namedId;
		params.cNamedArgs = 1;
	}
	EXCEPINFO exception{};
	UINT argumentError = 0;
	VARIANT localResult;
	VariantInit(&localResult);
	hr = object->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT, flags, &params, result ? result : &localResult,
			    &exception, &argumentError);
	for (VARIANTARG &argument : reversed)
		VariantClear(&argument);
	if (!result)
		VariantClear(&localResult);
	if (FAILED(hr)) {
		const QString description = exception.bstrDescription
						    ? QString::fromWCharArray(exception.bstrDescription)
						    : HResultMessage(hr);
		error = QStringLiteral("%1: %2").arg(QString::fromWCharArray(name), description);
		if (exception.bstrSource)
			SysFreeString(exception.bstrSource);
		if (exception.bstrDescription)
			SysFreeString(exception.bstrDescription);
		if (exception.bstrHelpFile)
			SysFreeString(exception.bstrHelpFile);
		return false;
	}
	return true;
}

QStringList NormalizePowerPointSlides(const QString &directoryPath, QString &error)
{
	QDir directory(directoryPath);
	QFileInfoList files = directory.entryInfoList({"*.png", "*.PNG"}, QDir::Files, QDir::Name);
	if (files.isEmpty()) {
		error = QStringLiteral("PowerPoint no generó imágenes de las diapositivas.");
		return {};
	}
	const QRegularExpression digits(QStringLiteral("(\\d+)"));
	std::sort(files.begin(), files.end(), [&digits](const QFileInfo &left, const QFileInfo &right) {
		const auto leftMatch = digits.match(left.completeBaseName());
		const auto rightMatch = digits.match(right.completeBaseName());
		return leftMatch.captured(1).toInt() < rightMatch.captured(1).toInt();
	});
	QStringList result;
	for (int index = 0; index < files.size(); ++index) {
		const QString destination = directory.filePath(QString::number(index + 1) + QStringLiteral(".png"));
		if (QFileInfo(files[index].absoluteFilePath()).absoluteFilePath() != QFileInfo(destination).absoluteFilePath()) {
			QFile::remove(destination);
			if (!QFile::rename(files[index].absoluteFilePath(), destination)) {
				error = QStringLiteral("No se pudo ordenar la diapositiva %1.").arg(index + 1);
				return {};
			}
		}
		result.push_back(destination);
	}
	return result;
}
} // namespace
#endif

PresentationImportResult PresentationImporter::ImportPdf(const QString &sourcePath,
							  const QString &destinationDirectory)
{
#ifdef _WIN32
	return std::async(std::launch::async, [sourcePath, destinationDirectory]() {
		PresentationImportResult result;
		try {
			winrt::init_apartment(winrt::apartment_type::multi_threaded);
			QDir().mkpath(destinationDirectory);
			QFile input(sourcePath);
			if (!input.open(QIODevice::ReadOnly)) {
				result.error = QStringLiteral("No se pudo abrir el archivo PDF.");
				return result;
			}
			const QByteArray inputBytes = input.readAll();
			if (inputBytes.isEmpty() ||
			    inputBytes.size() > static_cast<qsizetype>(std::numeric_limits<uint32_t>::max())) {
				result.error = QStringLiteral("El archivo PDF está vacío o es demasiado grande.");
				return result;
			}
			winrt::Windows::Storage::Streams::InMemoryRandomAccessStream pdfStream;
			const auto outputStream = pdfStream.GetOutputStreamAt(0);
			winrt::Windows::Storage::Streams::DataWriter writer(outputStream);
			writer.WriteBytes(winrt::array_view<const uint8_t>(
				reinterpret_cast<const uint8_t *>(inputBytes.constData()),
				reinterpret_cast<const uint8_t *>(inputBytes.constData()) + inputBytes.size()));
			writer.StoreAsync().get();
			writer.FlushAsync().get();
			writer.DetachStream();
			pdfStream.Seek(0);
			const auto document =
				winrt::Windows::Data::Pdf::PdfDocument::LoadFromStreamAsync(pdfStream).get();
			if (document.PageCount() == 0) {
				result.error = QStringLiteral("El PDF no contiene páginas.");
				return result;
			}
			for (uint32_t index = 0; index < document.PageCount(); ++index) {
				const auto page = document.GetPage(index);
				const auto pageSize = page.Size();
				winrt::Windows::Data::Pdf::PdfPageRenderOptions options;
				const uint32_t width = 1920;
				const uint32_t height = std::max(
					1u, static_cast<uint32_t>(std::lround(width * pageSize.Height / pageSize.Width)));
				options.DestinationWidth(width);
				options.DestinationHeight(height);
				winrt::Windows::Storage::Streams::InMemoryRandomAccessStream stream;
				page.RenderToStreamAsync(stream, options).get();
				const uint32_t byteCount = static_cast<uint32_t>(stream.Size());
				const auto input = stream.GetInputStreamAt(0);
				winrt::Windows::Storage::Streams::DataReader reader(input);
				reader.LoadAsync(byteCount).get();
				std::vector<uint8_t> bytes(byteCount);
				reader.ReadBytes(bytes);
				const QString path =
					QDir(destinationDirectory).filePath(QString::number(index + 1) + ".png");
				QFile output(path);
				if (!output.open(QIODevice::WriteOnly) ||
				    output.write(reinterpret_cast<const char *>(bytes.data()), bytes.size()) !=
					    static_cast<qint64>(bytes.size())) {
					result.error =
						QStringLiteral("No se pudo guardar la página %1 del PDF.").arg(index + 1);
					return result;
				}
				result.slidePaths.push_back(path);
			}
			result.success = true;
		} catch (const winrt::hresult_error &exception) {
			result.error = QString::fromWCharArray(exception.message().c_str());
		} catch (const std::exception &exception) {
			result.error = QString::fromUtf8(exception.what());
		}
		return result;
	}).get();
#else
	return {false, {}, QStringLiteral("La importación PDF está disponible actualmente en Windows.")};
#endif
}

PresentationImportResult PresentationImporter::ImportPowerPoint(const QString &sourcePath,
								 const QString &destinationDirectory)
{
#ifdef _WIN32
	return std::async(std::launch::async, [sourcePath, destinationDirectory]() {
		PresentationImportResult result;
		const HRESULT initialized = OleInitialize(nullptr);
		if (FAILED(initialized) && initialized != RPC_E_CHANGED_MODE) {
			result.error = HResultMessage(initialized);
			return result;
		}
		CLSID powerPointClass{};
		HRESULT hr = CLSIDFromProgID(L"PowerPoint.Application", &powerPointClass);
		if (FAILED(hr)) {
			result.error = QStringLiteral(
				"Microsoft PowerPoint no está instalado. Instálalo o exporta la presentación como PDF.");
			if (SUCCEEDED(initialized))
				OleUninitialize();
			return result;
		}
		ComPtr<IDispatch> application;
		hr = CoCreateInstance(powerPointClass, nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&application));
		if (FAILED(hr)) {
			result.error = QStringLiteral("No se pudo iniciar Microsoft PowerPoint: %1").arg(HResultMessage(hr));
			if (SUCCEEDED(initialized))
				OleUninitialize();
			return result;
		}

		QString error;
		VARIANT presentationsResult;
		VariantInit(&presentationsResult);
		std::vector<VARIANT> noArguments;
		if (!Invoke(application.Get(), L"Presentations", DISPATCH_PROPERTYGET, noArguments,
			    &presentationsResult, error) ||
		    presentationsResult.vt != VT_DISPATCH) {
			result.error = error.isEmpty() ? QStringLiteral("PowerPoint no expuso sus presentaciones.") : error;
			VariantClear(&presentationsResult);
			if (SUCCEEDED(initialized))
				OleUninitialize();
			return result;
		}
		ComPtr<IDispatch> presentations;
		presentations.Attach(presentationsResult.pdispVal);
		presentationsResult.vt = VT_EMPTY;

		std::vector<VARIANT> openArguments;
		openArguments.push_back(StringVariant(QFileInfo(sourcePath).absoluteFilePath()));
		openArguments.push_back(BoolVariant(true));
		openArguments.push_back(BoolVariant(false));
		openArguments.push_back(BoolVariant(false));
		VARIANT presentationResult;
		VariantInit(&presentationResult);
		const bool opened = Invoke(presentations.Get(), L"Open", DISPATCH_METHOD, openArguments,
					   &presentationResult, error);
		for (VARIANT &argument : openArguments)
			VariantClear(&argument);
		if (!opened || presentationResult.vt != VT_DISPATCH) {
			result.error = error.isEmpty() ? QStringLiteral("PowerPoint no pudo abrir el archivo.") : error;
		} else {
			ComPtr<IDispatch> presentation;
			presentation.Attach(presentationResult.pdispVal);
			presentationResult.vt = VT_EMPTY;
			QDir().mkpath(destinationDirectory);
			std::vector<VARIANT> exportArguments;
			exportArguments.push_back(StringVariant(QDir(destinationDirectory).absolutePath()));
			exportArguments.push_back(StringVariant(QStringLiteral("PNG")));
			exportArguments.push_back(IntVariant(1920));
			exportArguments.push_back(IntVariant(1080));
			if (!Invoke(presentation.Get(), L"Export", DISPATCH_METHOD, exportArguments, nullptr, error)) {
				result.error = error;
			} else {
				result.slidePaths = NormalizePowerPointSlides(destinationDirectory, result.error);
				result.success = !result.slidePaths.isEmpty();
			}
			for (VARIANT &argument : exportArguments)
				VariantClear(&argument);
			Invoke(presentation.Get(), L"Close", DISPATCH_METHOD, noArguments, nullptr, error);
		}
		Invoke(application.Get(), L"Quit", DISPATCH_METHOD, noArguments, nullptr, error);
		if (SUCCEEDED(initialized))
			OleUninitialize();
		return result;
	}).get();
#else
	return {false, {}, QStringLiteral("La importación PowerPoint está disponible actualmente en Windows.")};
#endif
}
