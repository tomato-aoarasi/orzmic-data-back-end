#pragma once

#ifndef __CXX20_SELF_TEST_HPP
#define __CXX20_SELF_TEST_HPP
#include "opencv2/opencv.hpp"
#include "ZXing/ReadBarcode.h"

#include "configuration/config.hpp"
#include <iostream>
#include <fstream>

namespace self::SelfTest {
	cv::Mat base64ToGrayScaleMat(std::string_view base64Str) {
		// 将base64字符串转换为字节数组
		std::vector<uchar> data(base64Str.begin(), base64Str.end());
		data = self::common::utils::base64Decode(base64Str.data());

		// 从字节数组中读取图像
		cv::Mat img{ cv::imdecode(data, cv::IMREAD_GRAYSCALE) };
		if (img.empty()) {
			img.release();
			throw self::HTTPException("base64 decode encountered an error", 500);
		}

		return std::move(img);
	};

	void qrcodeTest(){
        cv::Mat qrcode{ base64ToGrayScaleMat("这个是base64")};
        
        if (qrcode.empty()) {
            throw std::runtime_error("Error: Unable to read the image file.");
        }
        
        int width{ qrcode.cols }, height{ qrcode.rows };
        std::uint8_t* data{ qrcode.data };
        // load your image data from somewhere. ImageFormat::Lum assumes grey scale image data.

        auto image = ZXing::ImageView(data, width, height, ZXing::ImageFormat::Lum);
        auto options = ZXing::DecodeHints().setFormats(ZXing::BarcodeFormat::QRCode);
        auto results = ZXing::ReadBarcodes(image, options);

        for (const auto& r : results){
            std::cout << ZXing::ToString(r.format()) << ": " << r.text() << "\n";
            std::cout << "\n\n" << std::endl;
        }
	}
}

#endif // !__CXX20_SELF_TEST_HPP
