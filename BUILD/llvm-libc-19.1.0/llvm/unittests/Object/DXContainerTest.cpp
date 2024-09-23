//===- DXContainerTest.cpp - Tests for DXContainerFile --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Object/DXContainer.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/BinaryFormat/Magic.h"
#include "llvm/ObjectYAML/DXContainerYAML.h"
#include "llvm/ObjectYAML/yaml2obj.h"
#include "llvm/Support/MemoryBufferRef.h"
#include "llvm/Testing/Support/Error.h"
#include "gtest/gtest.h"

using namespace llvm;
using namespace llvm::object;

template <std::size_t X> MemoryBufferRef getMemoryBuffer(uint8_t Data[X]) {
  StringRef Obj(reinterpret_cast<char *>(&Data[0]), X);
  return MemoryBufferRef(Obj, "");
}

TEST(DXCFile, IdentifyMagic) {
  {
    StringRef Buffer("DXBC");
    EXPECT_EQ(identify_magic(Buffer), file_magic::dxcontainer_object);
  }
  {
    StringRef Buffer("DXBCBlahBlahBlah");
    EXPECT_EQ(identify_magic(Buffer), file_magic::dxcontainer_object);
  }
}

TEST(DXCFile, ParseHeaderErrors) {
  uint8_t Buffer[] = {0x44, 0x58, 0x42, 0x43};
  EXPECT_THAT_EXPECTED(
      DXContainer::create(getMemoryBuffer<4>(Buffer)),
      FailedWithMessage("Reading structure out of file bounds"));
}

TEST(DXCFile, EmptyFile) {
  EXPECT_THAT_EXPECTED(
      DXContainer::create(MemoryBufferRef(StringRef("", 0), "")),
      FailedWithMessage("Reading structure out of file bounds"));
}

TEST(DXCFile, ParseHeader) {
  uint8_t Buffer[] = {0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                      0x70, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  DXContainer C =
      llvm::cantFail(DXContainer::create(getMemoryBuffer<32>(Buffer)));
  EXPECT_TRUE(memcmp(C.getHeader().Magic, "DXBC", 4) == 0);
  EXPECT_TRUE(memcmp(C.getHeader().FileHash.Digest,
                     "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16) == 0);
  EXPECT_EQ(C.getHeader().Version.Major, 1u);
  EXPECT_EQ(C.getHeader().Version.Minor, 0u);
}

TEST(DXCFile, ParsePartMissingOffsets) {
  uint8_t Buffer[] = {
      0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x70, 0x0D, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  };
  EXPECT_THAT_EXPECTED(
      DXContainer::create(getMemoryBuffer<32>(Buffer)),
      FailedWithMessage("Reading structure out of file bounds"));
}

TEST(DXCFile, ParsePartInvalidOffsets) {
  // This test covers a case where the part offset is beyond the buffer size.
  uint8_t Buffer[] = {
      0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x70, 0x0D, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
  };
  EXPECT_THAT_EXPECTED(
      DXContainer::create(getMemoryBuffer<36>(Buffer)),
      FailedWithMessage("Part offset points beyond boundary of the file"));
}

TEST(DXCFile, ParsePartTooSmallBuffer) {
  // This test covers a case where there is insufficent space to read a full
  // part name, but the offset for the part is inside the buffer.
  uint8_t Buffer[] = {0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                      0x26, 0x0D, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                      0x24, 0x00, 0x00, 0x00, 0x46, 0x4B};
  EXPECT_THAT_EXPECTED(
      DXContainer::create(getMemoryBuffer<38>(Buffer)),
      FailedWithMessage("File not large enough to read part name"));
}

TEST(DXCFile, ParsePartNoSize) {
  // This test covers a case where the part's header is readable, but the size
  // the part extends beyond the boundaries of the file.
  uint8_t Buffer[] = {0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x28, 0x0D, 0x00,
                      0x00, 0x01, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00,
                      0x46, 0x4B, 0x45, 0x30, 0x00, 0x00};
  EXPECT_THAT_EXPECTED(
      DXContainer::create(getMemoryBuffer<42>(Buffer)),
      FailedWithMessage("Reading part size out of file bounds"));
}

TEST(DXCFile, ParseOverlappingParts) {
  // This test covers a case where a part's offset is inside the size range
  // covered by the previous part.
  uint8_t Buffer[] = {
      0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x40, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,
      0x2C, 0x00, 0x00, 0x00, 0x46, 0x4B, 0x45, 0x30, 0x08, 0x00, 0x00, 0x00,
      0x46, 0x4B, 0x45, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  EXPECT_THAT_EXPECTED(
      DXContainer::create(getMemoryBuffer<60>(Buffer)),
      FailedWithMessage(
          "Part offset for part 1 begins before the previous part ends"));
}

TEST(DXCFile, ParseEmptyParts) {
  uint8_t Buffer[] = {
      0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x70, 0x0D, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00,
      0x44, 0x00, 0x00, 0x00, 0x4C, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00,
      0x5C, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x6C, 0x00, 0x00, 0x00,
      0x46, 0x4B, 0x45, 0x30, 0x00, 0x00, 0x00, 0x00, 0x46, 0x4B, 0x45, 0x31,
      0x00, 0x00, 0x00, 0x00, 0x46, 0x4B, 0x45, 0x32, 0x00, 0x00, 0x00, 0x00,
      0x46, 0x4B, 0x45, 0x33, 0x00, 0x00, 0x00, 0x00, 0x46, 0x4B, 0x45, 0x34,
      0x00, 0x00, 0x00, 0x00, 0x46, 0x4B, 0x45, 0x35, 0x00, 0x00, 0x00, 0x00,
      0x46, 0x4B, 0x45, 0x36, 0x00, 0x00, 0x00, 0x00,
  };
  DXContainer C =
      llvm::cantFail(DXContainer::create(getMemoryBuffer<116>(Buffer)));
  EXPECT_EQ(C.getHeader().PartCount, 7u);

  // All the part sizes are 0, which makes a nice test of the range based for
  int ElementsVisited = 0;
  for (auto Part : C) {
    EXPECT_EQ(Part.Part.Size, 0u);
    EXPECT_EQ(Part.Data.size(), 0u);
    ++ElementsVisited;
  }
  EXPECT_EQ(ElementsVisited, 7);

  {
    // These are all intended to be fake part names so that the parser doesn't
    // try to parse the part data.
    auto It = C.begin();
    EXPECT_TRUE(memcmp(It->Part.Name, "FKE0", 4) == 0);
    ++It;
    EXPECT_TRUE(memcmp(It->Part.Name, "FKE1", 4) == 0);
    ++It;
    EXPECT_TRUE(memcmp(It->Part.Name, "FKE2", 4) == 0);
    ++It;
    EXPECT_TRUE(memcmp(It->Part.Name, "FKE3", 4) == 0);
    ++It;
    EXPECT_TRUE(memcmp(It->Part.Name, "FKE4", 4) == 0);
    ++It;
    EXPECT_TRUE(memcmp(It->Part.Name, "FKE5", 4) == 0);
    ++It;
    EXPECT_TRUE(memcmp(It->Part.Name, "FKE6", 4) == 0);
    ++It; // Don't increment past the end
    EXPECT_TRUE(memcmp(It->Part.Name, "FKE6", 4) == 0);
  }
}

// This test verify DXIL part are correctly parsed.
// This test is based on the binary output constructed from this yaml.
// --- !dxcontainer
// Header:
//   Hash:            [ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
//                      0x0, 0x0, 0x0, 0x0, 0x0, 0x0 ]
//   Version:
//     Major:           1
//     Minor:           0
//   PartCount:       1
// Parts:
//   - Name:            DXIL
//     Size:            28
//     Program:
//       MajorVersion:    6
//       MinorVersion:    5
//       ShaderKind:      5
//       Size:            8
//       DXILMajorVersion: 1
//       DXILMinorVersion: 5
//       DXILSize:        4
//       DXIL:            [ 0x42, 0x43, 0xC0, 0xDE, ]
// ...
TEST(DXCFile, ParseDXILPart) {
  uint8_t Buffer[] = {
      0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x48, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00,
      0x44, 0x58, 0x49, 0x4c, 0x1c, 0x00, 0x00, 0x00, 0x65, 0x00, 0x05, 0x00,
      0x08, 0x00, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4c, 0x05, 0x01, 0x00, 0x00,
      0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x42, 0x43, 0xc0, 0xde};
  DXContainer C =
      llvm::cantFail(DXContainer::create(getMemoryBuffer<116>(Buffer)));
  EXPECT_EQ(C.getHeader().PartCount, 1u);
  const std::optional<object::DXContainer::DXILData> &DXIL = C.getDXIL();
  EXPECT_TRUE(DXIL.has_value());
  dxbc::ProgramHeader Header = DXIL->first;
  EXPECT_EQ(Header.getMajorVersion(), 6u);
  EXPECT_EQ(Header.getMinorVersion(), 5u);
  EXPECT_EQ(Header.ShaderKind, 5u);
  EXPECT_EQ(Header.Size, 8u);
  EXPECT_EQ(Header.Bitcode.MajorVersion, 1u);
  EXPECT_EQ(Header.Bitcode.MinorVersion, 5u);
}

static Expected<DXContainer>
generateDXContainer(StringRef Yaml, SmallVectorImpl<char> &BinaryData) {
  DXContainerYAML::Object Obj;
  SMDiagnostic GenerateDiag;
  yaml::Input YIn(
      Yaml, /*Ctxt=*/nullptr,
      [](const SMDiagnostic &Diag, void *DiagContext) {
        *static_cast<SMDiagnostic *>(DiagContext) = Diag;
      },
      &GenerateDiag);

  YIn >> Obj;
  if (YIn.error())
    return createStringError(YIn.error(), GenerateDiag.getMessage());

  raw_svector_ostream OS(BinaryData);
  std::string ErrorMsg;
  if (!yaml::yaml2dxcontainer(
          Obj, OS, [&ErrorMsg](const Twine &Msg) { ErrorMsg = Msg.str(); }))
    return createStringError(YIn.error(), ErrorMsg);

  MemoryBufferRef BinaryDataRef = MemoryBufferRef(OS.str(), "");

  return DXContainer::create(BinaryDataRef);
}

TEST(DXCFile, PSVResourceIterators) {
  const char *Yaml = R"(
--- !dxcontainer
Header:
  Hash:            [ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
                     0x0, 0x0, 0x0, 0x0, 0x0, 0x0 ]
  Version:
    Major:           1
    Minor:           0
  PartCount:       2
Parts:
  - Name:            PSV0
    Size:            144
    PSVInfo:
      Version:         0
      ShaderStage:     14
      PayloadSizeInBytes: 4092
      MinimumWaveLaneCount: 0
      MaximumWaveLaneCount: 4294967295
      ResourceStride:  16
      Resources:
        - Type:            Sampler
          Space:           1
          LowerBound:      1
          UpperBound:      1
        - Type:            CBV
          Space:           2
          LowerBound:      2
          UpperBound:      2
        - Type:            SRVTyped
          Space:           3
          LowerBound:      3
          UpperBound:      3
  - Name:            DXIL
    Size:            24
    Program:
      MajorVersion:    6
      MinorVersion:    0
      ShaderKind:      14
      Size:            6
      DXILMajorVersion: 1
      DXILMinorVersion: 0
      DXILSize:        0
...
)";

  SmallVector<char, 256> BinaryData;
  auto C = generateDXContainer(Yaml, BinaryData);

  ASSERT_THAT_EXPECTED(C, Succeeded());

  const auto &PSVInfo = C->getPSVInfo();
  ASSERT_TRUE(PSVInfo.has_value());

  EXPECT_EQ(PSVInfo->getResourceCount(), 3u);

  auto It = PSVInfo->getResources().begin();

  EXPECT_TRUE(It == PSVInfo->getResources().begin());

  dxbc::PSV::v2::ResourceBindInfo Binding;

  Binding = *It;
  EXPECT_EQ(Binding.Type, dxbc::PSV::ResourceType::Sampler);
  EXPECT_EQ(Binding.Flags, 0u);

  ++It;
  Binding = *It;

  EXPECT_EQ(Binding.Type, dxbc::PSV::ResourceType::CBV);
  EXPECT_EQ(Binding.Flags, 0u);

  --It;
  Binding = *It;

  EXPECT_TRUE(It == PSVInfo->getResources().begin());

  EXPECT_EQ(Binding.Type, dxbc::PSV::ResourceType::Sampler);
  EXPECT_EQ(Binding.Flags, 0u);

  --It;
  Binding = *It;

  EXPECT_EQ(Binding.Type, dxbc::PSV::ResourceType::Sampler);
  EXPECT_EQ(Binding.Flags, 0u);

  ++It;
  Binding = *It;

  EXPECT_EQ(Binding.Type, dxbc::PSV::ResourceType::CBV);
  EXPECT_EQ(Binding.Flags, 0u);

  ++It;
  Binding = *It;

  EXPECT_EQ(Binding.Type, dxbc::PSV::ResourceType::SRVTyped);
  EXPECT_EQ(Binding.Flags, 0u);

  EXPECT_FALSE(It == PSVInfo->getResources().end());

  ++It;
  Binding = *It;

  EXPECT_TRUE(It == PSVInfo->getResources().end());
  EXPECT_FALSE(It != PSVInfo->getResources().end());

  EXPECT_EQ(Binding.Type, dxbc::PSV::ResourceType::Invalid);
  EXPECT_EQ(Binding.Flags, 0u);

  {
    auto Old = It++;
    Binding = *Old;

    EXPECT_TRUE(Old == PSVInfo->getResources().end());
    EXPECT_FALSE(Old != PSVInfo->getResources().end());

    EXPECT_EQ(Binding.Type, dxbc::PSV::ResourceType::Invalid);
    EXPECT_EQ(Binding.Flags, 0u);
  }

  Binding = *It;

  EXPECT_TRUE(It == PSVInfo->getResources().end());

  EXPECT_EQ(Binding.Type, dxbc::PSV::ResourceType::Invalid);
  EXPECT_EQ(Binding.Flags, 0u);

  {
    auto Old = It--;
    Binding = *Old;
    EXPECT_TRUE(Old == PSVInfo->getResources().end());

    EXPECT_EQ(Binding.Type, dxbc::PSV::ResourceType::Invalid);
    EXPECT_EQ(Binding.Flags, 0u);
  }

  Binding = *It;

  EXPECT_EQ(Binding.Type, dxbc::PSV::ResourceType::SRVTyped);
  EXPECT_EQ(Binding.Flags, 0u);
}

// The malicious file bits in these tests are mutations of the binary produced
// by the following YAML:
//
// --- !dxcontainer
// Header:
//   Hash:            [ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
//                      0x0, 0x0, 0x0, 0x0, 0x0, 0x0 ]
//   Version:
//     Major:           1
//     Minor:           0
//   PartCount:       3
// Parts:
//   - Name:            DXIL
//     Size:            24
//     Program:
//       MajorVersion:    6
//       MinorVersion:    0
//       ShaderKind:      14
//       Size:            6
//       DXILMajorVersion: 1
//       DXILMinorVersion: 0
//       DXILSize:        0
//   - Name:            PSV0
//     Size:            36
//     PSVInfo:
//       Version:         0
//       ShaderStage:     5
//       MinimumWaveLaneCount: 0
//       MaximumWaveLaneCount: 0
//       ResourceStride:       16
//       Resources: []
//   - Name: BLEH
//     Size: 16
// ...

TEST(DXCFile, MaliciousFiles) {

  // In this file blob, the file size is specified as 96 bytes (0x60), and the
  // PSV0 data is specified as 24 bytes (0x18) which extends beyond the size of
  // the file.
  {
    uint8_t Buffer[] = {
        0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x60, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,
        0x48, 0x00, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4C, 0x18, 0x00, 0x00, 0x00,
        0x60, 0x00, 0x0E, 0x00, 0x06, 0x00, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4C,
        0x00, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x50, 0x53, 0x56, 0x30, 0x24, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    EXPECT_THAT_EXPECTED(
        DXContainer::create(getMemoryBuffer<96>(Buffer)),
        FailedWithMessage(
            "Pipeline state data extends beyond the bounds of the part"));
  }

  // PSV extends beyond part, but in file range. In this blob the file size is
  // 144 bytes (0x90), and the PSV part is 36 bytes (0x24), and the PSV data is
  // 40 bytes (0x40).
  {
    uint8_t Buffer[] = {
        0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x90, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x2C, 0x00, 0x00, 0x00,
        0x4C, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4C,
        0x18, 0x00, 0x00, 0x00, 0x60, 0x00, 0x0E, 0x00, 0x06, 0x00, 0x00, 0x00,
        0x44, 0x58, 0x49, 0x4C, 0x00, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x50, 0x53, 0x56, 0x30, 0x24, 0x00, 0x00, 0x00,
        0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
        0x42, 0x4C, 0x45, 0x48, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    EXPECT_THAT_EXPECTED(
        DXContainer::create(getMemoryBuffer<144>(Buffer)),
        FailedWithMessage(
            "Pipeline state data extends beyond the bounds of the part"));
  }

  // In this file blob, the file is 116 bytes (0x74). The file structure is
  // valid except that it specifies 1 16 byte resource binding which would
  // extend beyond the range of the part and file.
  {
    uint8_t Buffer[] = {
        0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x74, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,
        0x48, 0x00, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4C, 0x18, 0x00, 0x00, 0x00,
        0x60, 0x00, 0x0E, 0x00, 0x06, 0x00, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4C,
        0x00, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x50, 0x53, 0x56, 0x30, 0x24, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
    };
    EXPECT_THAT_EXPECTED(
        DXContainer::create(getMemoryBuffer<116>(Buffer)),
        FailedWithMessage(
            "Resource binding data extends beyond the bounds of the part"));
  }

  // In this file blob, the file is 116 bytes (0x74). The file structure is
  // valid except that it specifies 1 16 byte resource binding which would
  // extend beyond the range of the part and into the `BLEH` part.
  {
    uint8_t Buffer[] = {
        0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x90, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x2C, 0x00, 0x00, 0x00,
        0x4C, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4C,
        0x18, 0x00, 0x00, 0x00, 0x60, 0x00, 0x0E, 0x00, 0x06, 0x00, 0x00, 0x00,
        0x44, 0x58, 0x49, 0x4C, 0x00, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x50, 0x53, 0x56, 0x30, 0x24, 0x00, 0x00, 0x00,
        0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
        0x42, 0x4C, 0x45, 0x48, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    EXPECT_THAT_EXPECTED(
        DXContainer::create(getMemoryBuffer<144>(Buffer)),
        FailedWithMessage(
            "Resource binding data extends beyond the bounds of the part"));
  }
}

// This test verifies that the resource iterator follows the stride even if the
// stride doesn't match an expected or known value. In this test, the resource
// data is structured validly, with 32 bytes per resource. This test is based on
// editing the binary output constructed from this yaml.
//
// --- !dxcontainer
// Header:
//   Hash:            [ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
//                      0x0, 0x0, 0x0, 0x0, 0x0, 0x0 ]
//   Version:
//     Major:           1
//     Minor:           0
//   PartCount:       2
// Parts:
//   - Name:            DXIL
//     Size:            24
//     Program:
//       MajorVersion:    6
//       MinorVersion:    0
//       ShaderKind:      14
//       Size:            6
//       DXILMajorVersion: 1
//       DXILMinorVersion: 0
//       DXILSize:        0
//   - Name:            PSV0
//     Size:            100
//     PSVInfo:
//       Version:         0
//       ShaderStage:     5
//       MinimumWaveLaneCount: 0
//       MaximumWaveLaneCount: 0
//       ResourceStride:       16
//       Resources:
//         - Type:            1
//           Space:           2
//           LowerBound:      3
//           UpperBound:      4
//         - Type:            5
//           Space:           6
//           LowerBound:      7
//           UpperBound:      8
// ...
TEST(DXCFile, PSVResourceIteratorsStride) {
  uint8_t Buffer[] = {
        0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xB0, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 
        0x28, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4C, 0x18, 0x00, 0x00, 0x00,
        0x60, 0x00, 0x0E, 0x00, 0x06, 0x00, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4C, 0x00, 0x01, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x53, 0x56, 0x30, 0x64, 0x00, 0x00, 0x00,
        0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
        0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
        0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
    }; 
    DXContainer C =
      llvm::cantFail(DXContainer::create(getMemoryBuffer<180>(Buffer)));

  const auto &PSVInfo = C.getPSVInfo();
  ASSERT_TRUE(PSVInfo.has_value());

  ASSERT_EQ(PSVInfo->getResourceCount(), 2u);

  auto It = PSVInfo->getResources().begin();

  EXPECT_TRUE(It == PSVInfo->getResources().begin());

  dxbc::PSV::v2::ResourceBindInfo Binding;

  Binding = *It;
  EXPECT_EQ(Binding.Type, dxbc::PSV::ResourceType::Sampler);
  EXPECT_EQ(Binding.Space, 2u);
  EXPECT_EQ(Binding.LowerBound, 3u);
  EXPECT_EQ(Binding.UpperBound, 4u);

  ++It;
  Binding = *It;

  EXPECT_EQ(Binding.Type, dxbc::PSV::ResourceType::SRVStructured);
  EXPECT_EQ(Binding.Space, 6u);
  EXPECT_EQ(Binding.LowerBound, 7u);
  EXPECT_EQ(Binding.UpperBound, 8u);

  --It;
  Binding = *It;

  EXPECT_TRUE(It == PSVInfo->getResources().begin());

  EXPECT_EQ(Binding.Type, dxbc::PSV::ResourceType::Sampler);
  EXPECT_EQ(Binding.Space, 2u);
  EXPECT_EQ(Binding.LowerBound, 3u);
  EXPECT_EQ(Binding.UpperBound, 4u);

  --It;
  Binding = *It;

  EXPECT_EQ(Binding.Type, dxbc::PSV::ResourceType::Sampler);
  EXPECT_EQ(Binding.Space, 2u);
  EXPECT_EQ(Binding.LowerBound, 3u);
  EXPECT_EQ(Binding.UpperBound, 4u);

  ++It;
  Binding = *It;

  EXPECT_EQ(Binding.Type, dxbc::PSV::ResourceType::SRVStructured);
  EXPECT_EQ(Binding.Space, 6u);
  EXPECT_EQ(Binding.LowerBound, 7u);
  EXPECT_EQ(Binding.UpperBound, 8u);;


  EXPECT_FALSE(It == PSVInfo->getResources().end());

  ++It;
  Binding = *It;

  EXPECT_TRUE(It == PSVInfo->getResources().end());
  EXPECT_FALSE(It != PSVInfo->getResources().end());

  EXPECT_EQ(Binding.Type, dxbc::PSV::ResourceType::Invalid);
  EXPECT_EQ(Binding.Flags, 0u);
}

// This test binary is created using mutations of the yaml in the SigElements
// test found under test/ObjectYAML/DXContainer/SigElements.yaml.

TEST(DXCFile, MisalignedStringTable) {
  uint8_t Buffer[] = {
      0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0xb4, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,
      0x48, 0x00, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4c, 0x18, 0x00, 0x00, 0x00,
      0x60, 0x00, 0x0e, 0x00, 0x06, 0x00, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4c,
      0x00, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x50, 0x53, 0x56, 0x30, 0x68, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
      0x05, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x08, 0x10, 0x20, 0x40,
      0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  EXPECT_THAT_EXPECTED(DXContainer::create(getMemoryBuffer<168>(Buffer)),
                       FailedWithMessage("String table misaligned"));
}

// This test binary is created using mutations of the yaml in the SigElements
// test found under test/ObjectYAML/DXContainer/SigElements.yaml.
TEST(DXCFile, SigElementsExtendBeyondPart) {
  uint8_t Buffer[] = {
      0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0xb4, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,
      0x48, 0x00, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4c, 0x18, 0x00, 0x00, 0x00,
      0x60, 0x00, 0x0e, 0x00, 0x06, 0x00, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4c,
      0x00, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x50, 0x53, 0x56, 0x30, 0x54, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
      0x05, 0x80, 0x00, 0x00, 0x02, 0x00, 0x00, 0x40, 0x08, 0x10, 0x20, 0x40,
      0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x49, 0x4e, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x02, 0x00, 0x42, 0x00, 0x02, 0x00, 0x03, 0x00};
  EXPECT_THAT_EXPECTED(
      DXContainer::create(getMemoryBuffer<164>(Buffer)),
      FailedWithMessage(
          "Signature elements extend beyond the size of the part"));
}

TEST(DXCFile, MalformedSignature) {
  /*
  The tests here exercise the DXContainer Signature section parser. These tests
  are based on modifying the binary described by the following yaml:

  --- !dxcontainer
  Header:
    Hash:            [ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                      0x0, 0x0, 0x0, 0x0, 0x0, 0x0 ]
    Version:
      Major:           1
      Minor:           0
    FileSize:        128
    PartCount:       1
    PartOffsets:     [ 64 ]
  Parts:
    - Name:            ISG1
      Size:            52
      Signature:
        Parameters:
          - Stream:          0
            Name:            AAA
            Index:           0
            SystemValue:     Undefined
            CompType:        Float32
            Register:        0
            Mask:            7
            ExclusiveMask:   2
            MinPrecision:    Default
  ...

  The unmodified hex sequence is:

  uint8_t Buffer[] = {
    0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x80,
  0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x49, 0x53, 0x47, 0x31, 0x34, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x07, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x41, 0x41, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  */

  {

    // This binary says the signature has 10 parameters, but the part data is
    // only big enough for 1.
    uint8_t Buffer[] = {
        0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x80, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x49, 0x53, 0x47, 0x31, 0x34, 0x00, 0x00, 0x00,
        0x0A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x02, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x41, 0x41, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00};
    EXPECT_THAT_EXPECTED(
        DXContainer::create(getMemoryBuffer<164>(Buffer)),
        FailedWithMessage(
            "Signature parameters extend beyond the part boundary"));
  }

  {

    // This binary only has one parameter, but the start offset is beyond the
    // size of the part.
    uint8_t Buffer[] = {
        0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x80, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x49, 0x53, 0x47, 0x31, 0x34, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x02, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x41, 0x41, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00};
    EXPECT_THAT_EXPECTED(
        DXContainer::create(getMemoryBuffer<164>(Buffer)),
        FailedWithMessage(
            "Signature parameters extend beyond the part boundary"));
  }

  {

    // This parameter has a name offset of 3, which is before the start of the
    // string table.
    uint8_t Buffer[] = {
        0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x80, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x49, 0x53, 0x47, 0x31, 0x34, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x02, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x41, 0x41, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00};
    EXPECT_THAT_EXPECTED(
        DXContainer::create(getMemoryBuffer<164>(Buffer)),
        FailedWithMessage("Invalid parameter name offset: name starts before "
                          "the first name offset"));
  }

  {
    // This parameter has a name offset of 255, which is after the end of the
    // part.
    uint8_t Buffer[] = {
        0x44, 0x58, 0x42, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x80, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x49, 0x53, 0x47, 0x31, 0x34, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x02, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x41, 0x41, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00};
    EXPECT_THAT_EXPECTED(
        DXContainer::create(getMemoryBuffer<164>(Buffer)),
        FailedWithMessage("Invalid parameter name offset: name starts after "
                          "the end of the part data"));
  }
}
