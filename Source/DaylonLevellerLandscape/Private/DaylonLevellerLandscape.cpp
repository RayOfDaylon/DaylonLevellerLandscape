// Copyright Daylon Graphics Ltd. All Rights Reserved.

#include "DaylonLevellerLandscape.h"
#include "LandscapeModule.h"
#include "LandscapeEditorModule.h"
#include "LandscapeFileFormatInterface.h"



#define LOCTEXT_NAMESPACE "FDaylonLevellerLandscapeModule"


DEFINE_LOG_CATEGORY(LogDaylonLevellerLandscape)


namespace Daylon
{
enum ECoordsys
{
	COORDSYS_RASTER = 0,
	COORDSYS_LOCAL  = 1,
	COORDSYS_GEO    = 2
};

enum EUnitLabel
{
    // Measurement unit IDs, OEM version.
    UNITLABEL_UNKNOWN = 0x00000000,
    UNITLABEL_PIXEL   = 0x70780000,
    UNITLABEL_PERCENT = 0x25000000,

    UNITLABEL_RADIAN    = 0x72616400,
    UNITLABEL_DEGREE    = 0x64656700,
    UNITLABEL_ARCMINUTE = 0x6172636D,
    UNITLABEL_ARCSECOND = 0x61726373,

    UNITLABEL_YM     = 0x796D0000,
    UNITLABEL_ZM     = 0x7A6D0000,
    UNITLABEL_AM     = 0x616D0000,
    UNITLABEL_FM     = 0x666D0000,
    UNITLABEL_PM     = 0x706D0000,
    UNITLABEL_A      = 0x41000000,
    UNITLABEL_NM     = 0x6E6D0000,
    UNITLABEL_U      = 0x75000000,
    UNITLABEL_UM     = 0x756D0000,
    UNITLABEL_PPT    = 0x70707400,
    UNITLABEL_PT     = 0x70740000,
    UNITLABEL_MM     = 0x6D6D0000,
    UNITLABEL_P      = 0x70000000,
    UNITLABEL_CM     = 0x636D0000,
    UNITLABEL_IN     = 0x696E0000,
    UNITLABEL_DFT    = 0x64667400,
    UNITLABEL_DM     = 0x646D0000,
    UNITLABEL_LI     = 0x6C690000,
    UNITLABEL_SLI    = 0x736C6900,
    UNITLABEL_SP     = 0x73700000,
    UNITLABEL_FT     = 0x66740000,
    UNITLABEL_SFT    = 0x73667400,
    UNITLABEL_YD     = 0x79640000,
    UNITLABEL_SYD    = 0x73796400,
    UNITLABEL_M      = 0x6D000000,
    UNITLABEL_FATH   = 0x66617468,
    UNITLABEL_R      = 0x72000000,
    UNITLABEL_RD     = UNITLABEL_R,
    UNITLABEL_DAM    = 0x64416D00,
    UNITLABEL_DKM    = UNITLABEL_DAM,
    UNITLABEL_CH     = 0x63680000,
    UNITLABEL_SCH    = 0x73636800,
    UNITLABEL_HM     = 0x686D0000,
    UNITLABEL_F      = 0x66000000,
    UNITLABEL_KM     = 0x6B6D0000,
    UNITLABEL_MI     = 0x6D690000,
    UNITLABEL_SMI    = 0x736D6900,
    UNITLABEL_NMI    = 0x6E6D6900,
    UNITLABEL_MEGAM  = 0x4D6D0000,
    UNITLABEL_LS     = 0x6C730000,
    UNITLABEL_GM     = 0x476D0000,
    UNITLABEL_LM     = 0x6C6D0000,
    UNITLABEL_AU     = 0x41550000,
    UNITLABEL_TM     = 0x546D0000,
    UNITLABEL_LHR    = 0x6C687200,
    UNITLABEL_LD     = 0x6C640000,
    UNITLABEL_PETAM  = 0x506D0000,
    UNITLABEL_LY     = 0x6C790000,
    UNITLABEL_PC     = 0x70630000,
    UNITLABEL_EXAM   = 0x456D0000,
    UNITLABEL_KLY    = 0x6B6C7900,
    UNITLABEL_KPC    = 0x6B706300,
    UNITLABEL_ZETTAM = 0x5A6D0000,
    UNITLABEL_MLY    = 0x4D6C7900,
    UNITLABEL_MPC    = 0x4D706300,
    UNITLABEL_YOTTAM = 0x596D0000
};

struct FMeasurementUnit
{
    const char*  ID;
    double       Scale;
    EUnitLabel   OemCode;
};

constexpr double kdays_per_year = 365.25;
constexpr double kdLStoM = 299792458.0;
constexpr double kdLYtoM = kdLStoM * kdays_per_year * 24 * 60 * 60;
constexpr double kdInch = 0.3048 / 12;
constexpr double kPI = UE_PI;

constexpr int kFirstLinearMeasureIdx = 9;

static const FMeasurementUnit kUnits[] = 
{
    {"",     1.0,                        UNITLABEL_UNKNOWN},
    {"px",   1.0,                        UNITLABEL_PIXEL},
    {"%",    1.0,                        UNITLABEL_PERCENT},  // not actually used

    {"rad",  1.0,                        UNITLABEL_RADIAN},
    {"\xB0", kPI / 180.0,                UNITLABEL_DEGREE},  // \xB0 is Unicode degree symbol
    {"d",    kPI / 180.0,                UNITLABEL_DEGREE},
    {"deg",  kPI / 180.0,                UNITLABEL_DEGREE},
    {"'",    kPI / (60.0 * 180.0),       UNITLABEL_ARCMINUTE},
    {"\"",   kPI / (3600.0 * 180.0),     UNITLABEL_ARCSECOND},

    {"ym",   1.0e-24,                    UNITLABEL_YM},
    {"zm",   1.0e-21,                    UNITLABEL_ZM},
    {"am",   1.0e-18,                    UNITLABEL_AM},
    {"fm",   1.0e-15,                    UNITLABEL_FM},
    {"pm",   1.0e-12,                    UNITLABEL_PM},
    {"A",    1.0e-10,                    UNITLABEL_A},
    {"nm",   1.0e-9,                     UNITLABEL_NM},
    {"u",    1.0e-6,                     UNITLABEL_U},
    {"um",   1.0e-6,                     UNITLABEL_UM},
    {"ppt",  kdInch / 72.27,             UNITLABEL_PPT},
    {"pt",   kdInch / 72.0,              UNITLABEL_PT},
    {"mm",   1.0e-3,                     UNITLABEL_MM},
    {"p",    kdInch / 6.0,               UNITLABEL_P},
    {"cm",   1.0e-2,                     UNITLABEL_CM},
    {"in",   kdInch,                     UNITLABEL_IN},
    {"dft",  0.03048,                    UNITLABEL_DFT},
    {"dm",   0.1,                        UNITLABEL_DM},
    {"li",   0.2011684 /* GDAL 0.20116684023368047 ? */, UNITLABEL_LI},
    {"sli",  0.201168402336805,          UNITLABEL_SLI},
    {"sp",   0.2286,                     UNITLABEL_SP},
    {"ft",   0.3048,                     UNITLABEL_FT},
    {"sft",  1200.0 / 3937.0,            UNITLABEL_SFT},
    {"yd",   0.9144,                     UNITLABEL_YD},
    {"syd",  0.914401828803658,          UNITLABEL_SYD},
    {"m",    1.0,                        UNITLABEL_M},
    {"fath", 1.8288,                     UNITLABEL_FATH},
    {"rd",   5.02921,                    UNITLABEL_RD},
    {"dam",  10.0,                       UNITLABEL_DAM},
    {"dkm",  10.0,                       UNITLABEL_DKM},
    {"ch",   20.1168 /* GDAL: 2.0116684023368047 ? */, UNITLABEL_CH},
    {"sch",  20.1168402336805,           UNITLABEL_SCH},
    {"hm",   100.0,                      UNITLABEL_HM},
    {"f",    201.168,                    UNITLABEL_F},
    {"km",   1000.0,                     UNITLABEL_KM},
    {"mi",   1609.344,                   UNITLABEL_MI},
    {"smi",  1609.34721869444,           UNITLABEL_SMI},
    {"nmi",  1853.0,                     UNITLABEL_NMI},
    {"Mm",   1.0e+6,                     UNITLABEL_MEGAM},
    {"ls",   kdLStoM,                    UNITLABEL_LS},
    {"Gm",   1.0e+9,                     UNITLABEL_GM},
    {"lm",   kdLStoM * 60,               UNITLABEL_LM},
    {"AU",   8.317 * kdLStoM * 60,       UNITLABEL_AU},
    {"Tm",   1.0e+12,                    UNITLABEL_TM},
    {"lhr",  60.0 * 60.0 * kdLStoM,      UNITLABEL_LHR},
    {"ld",   24 * 60.0 * 60.0 * kdLStoM, UNITLABEL_LD},
    {"Pm",   1.0e+15,                    UNITLABEL_PETAM},
    {"ly",   kdLYtoM,                    UNITLABEL_LY},
    {"pc",   3.2616 * kdLYtoM,           UNITLABEL_PC},
    {"Em",   1.0e+18,                    UNITLABEL_EXAM},
    {"kly",  1.0e+3 * kdLYtoM,           UNITLABEL_KLY},
    {"kpc",  3.2616 * 1.0e+3 * kdLYtoM,  UNITLABEL_KPC},
    {"Zm",   1.0e+21,                    UNITLABEL_ZETTAM},
    {"Mly",  1.0e+6 * kdLYtoM,           UNITLABEL_MLY},
    {"Mpc",  3.2616 * 1.0e+6 * kdLYtoM,  UNITLABEL_MPC},
    {"Ym",   1.0e+24,                    UNITLABEL_YOTTAM}
};


class FLevellerDocument
{
	uint64          Mark = 0;
	
	public:

		TArray64<uint8> Contents;
		uint64          DataOffset = 0;
		uint64          DataLength = 0;
		float           SpanLow    = 0.0f;
		float           SpanHi     = 0.0f;
		int32           Width      = 0;
		int32           Breadth    = 0;

		FLandscapeFileInfo       Init         (const TCHAR* Filename);
		void                     ComputeSpan  ();
		bool                     Read         (uint64 Length, void* Buffer);
		bool                     LocateData   (const char* Tag, uint64& Offset, uint64& Length);
		bool                     Get          (const char* Tag, int32& N, FLandscapeFileInfo& Result);
		bool                     Get          (const char* Tag, double& N, FLandscapeFileInfo& Result);
		const FMeasurementUnit*  GetUnit      (int32 Code) const;
		double                   ToMeters     (double Measure, int32 FromUnits);
};


enum EAxisExtent
{
    // Digital axis extent styles.
    DA_POSITIONED = 0,
    DA_SIZED,
    DA_PIXEL_SIZED
};


struct FDigitalAxis
{
    bool Get(FLevellerDocument& Doc, int n)
    {
		FLandscapeFileInfo Result;

        char szTag[32];

        snprintf(szTag, sizeof(szTag), "coordsys_da%d_style", n);
        if (!Doc.Get(szTag, (int32&)Style, Result))
            return false;

        snprintf(szTag, sizeof(szTag), "coordsys_da%d_fixedend", n);
        if (!Doc.Get(szTag, FixedEnd, Result))
            return false;

        snprintf(szTag, sizeof(szTag), "coordsys_da%d_v0", n);
        if (!Doc.Get(szTag, D[0], Result))
            return false;

        snprintf(szTag, sizeof(szTag), "coordsys_da%d_v1", n);
        if (!Doc.Get(szTag, D[1], Result))
            return false;

        return true;
    }

    double Origin(int32 Pixels) const
    {
        if (FixedEnd == 1)
        {
            switch(Style)
            {
                case DA_SIZED:
                    return D[1] + D[0];

                case DA_PIXEL_SIZED:
                    return D[1] + (D[0] * (Pixels - 1));
            }
        }
        return D[0];
    }

    double Scaling(int32 Pixels) const
    {
        if(Pixels <= 1)
		{
			return 1.0;
		}

        if (Style == DA_PIXEL_SIZED)
		{
            return D[1 - FixedEnd];
		}

        return Length(Pixels) / (Pixels - 1);
    }

    double Length(int32 Pixels) const
    {
        // Return the signed length of the axis.

        switch(Style)
        {
            case DA_POSITIONED:  return D[1] - D[0];
            case DA_SIZED:       return D[1 - FixedEnd];
            case DA_PIXEL_SIZED: return D[1 - FixedEnd] * (Pixels - 1);
        }
        
        return 0.0;
    }

  private:

    EAxisExtent   Style     = DA_PIXEL_SIZED;
    int32         FixedEnd  = 0;
    double        D[2] = { 0.0, 0.0 };
};

} // namespace Daylon



const Daylon::FMeasurementUnit* Daylon::FLevellerDocument::GetUnit(int32 Code) const
{
	for(const auto& Unit : kUnits)
	{
		if(Unit.OemCode == Code)
		{
			return &Unit;
		}
	}

	return nullptr;
}

double Daylon::FLevellerDocument::ToMeters(double Measure, int32 FromUnits)
{
	auto Unit = GetUnit(FromUnits);
	if(Unit == nullptr)
	{
		return 1.0;
	}

	return Measure * Unit->Scale;
}


FLandscapeFileInfo Daylon::FLevellerDocument::Init(const TCHAR* Filename)
{
	FLandscapeFileInfo Result;
	Result.ResultCode = ELandscapeImportResult::Success;

	if (!FFileHelper::LoadFileToArray(Contents, Filename, FILEREAD_Silent))
	{
		Result.ResultCode = ELandscapeImportResult::Error;
		Result.ErrorMessage = LOCTEXT("DaylonLeveller_DocOpenError", "Error opening Leveller document");
	}
	else if(::memcmp(Contents.GetData(), "trrn", 4) != 0)
	{
		Result.ResultCode = ELandscapeImportResult::Error;
		Result.ErrorMessage = LOCTEXT("DaylonLeveller_DocTypeError", "Heightfield file is not a Leveller document");
	}
	else if(Contents[4] < 7 || Contents[4] > 12)
	{
		Result.ResultCode = ELandscapeImportResult::Error;
		Result.ErrorMessage = LOCTEXT("DaylonLeveller_DocVersionError", "Leveller document version unsupported");
	}

	// Header okay. Let's try some core tags.

	if(!Get("hf_w", Width,   Result)) { return Result; }
	if(!Get("hf_b", Breadth, Result)) { return Result; }

	if(Width < 0 || Breadth < 0 || !LocateData("hf_data", DataOffset, DataLength) || Width * Breadth * (int32)sizeof(float) >= Contents.Num())
	{
		Result.ResultCode = ELandscapeImportResult::Error;
		Result.ErrorMessage = LOCTEXT("DaylonLeveller_BadDocError", "Leveller document invalid");
		return Result;
	}

	if(Width > 8192 || Breadth > 8192)
	{
		Result.ResultCode = ELandscapeImportResult::Error;
		Result.ErrorMessage = LOCTEXT("DaylonLeveller_DocTooBigError", "Leveller document too large");
		return Result;
	}

	FLandscapeFileResolution ImportResolution;
	ImportResolution.Width  = static_cast<uint32>(Width);
	ImportResolution.Height = static_cast<uint32>(Breadth);
	Result.PossibleResolutions.Add(ImportResolution);

	ComputeSpan();


	// todo: we're assuming 1m per px elevations, so fix this to use actual per-pixel elev measure.
	ECoordsys CoordsysType = COORDSYS_RASTER;
	if(!Get("csclass", (int32&)CoordsysType, Result))
	{
		Result.ResultCode = ELandscapeImportResult::Error;
		Result.ErrorMessage = LOCTEXT("DaylonLeveller_BadDocError", "Can't determine coordinate system");
		return Result;
	}

	switch(CoordsysType)
	{
		case COORDSYS_RASTER:
			// Assume vertices are 1.0 meters apart, and elevations are 1 m/px.
			Result.DataScale = FVector(100.0f, 100.0f, FMath::Max(1.0f, (SpanHi - SpanLow) / (255.992f + 256.0f) * 100.0f));
			break;

		case COORDSYS_LOCAL:
		{
			float GroundWidthSpacingMeters, GroundBreadthSpacingMeters, SpanMeters = 1.0f;

			{
				int32 UnitCode;

				if (!Get("coordsys_units", UnitCode, Result))
				{
					Result.ResultCode = ELandscapeImportResult::Error;
					Result.ErrorMessage = LOCTEXT("DaylonLeveller_BadDocError", "Can't determine measurement unit");
					return Result;
				}

				FDigitalAxis Axis_ns, Axis_ew;

				if (Axis_ns.Get(*this,  0) && Axis_ew.Get(*this,  1))
				{
					GroundWidthSpacingMeters   = ToMeters(Axis_ew.Scaling(Width), UnitCode);
					GroundBreadthSpacingMeters = ToMeters(Axis_ns.Scaling(Breadth), UnitCode);
				}
			}

			// Get vertical (elev) coordsys.
			int32 bHasVertCS = false;

			if(Get("coordsys_haselevm", bHasVertCS, Result) && bHasVertCS)
			{
				double ElevScale;//, ElevBase;
				Get("coordsys_em_scale", ElevScale, Result);
				//Get("coordsys_em_base", ElevBase, Result);

				EUnitLabel ElevUnitCode;

				if (Get("coordsys_em_units", (int32&)ElevUnitCode, Result))
				{
					// To get m per px, we need to 
					auto ElevMetersPerPixel = ToMeters(ElevScale, ElevUnitCode);
					SpanMeters = (float)((SpanHi - SpanLow) * ElevMetersPerPixel);
				}
			}

			Result.DataScale = FVector(GroundWidthSpacingMeters * 100.0f, GroundBreadthSpacingMeters * 100.0f, FMath::Max(1.0f, SpanMeters / (255.992f + 256.0f) * 100.0f));

			// Log scale for user's reference.
			const auto Scale = Result.DataScale.GetValue();

			UE_LOG(LogDaylonLevellerLandscape, Log, TEXT("Computed landscape scale: %.3f, %.3f, %.3f"), Scale.X, Scale.Y, Scale.Z);
		}
			break;

		//case COORDSYS_GEO:
			// We would need to parse the WKT string which we currently can't do.

		default:
			Result.ResultCode = ELandscapeImportResult::Error;
			Result.ErrorMessage = LOCTEXT("DaylonLeveller_BadDocError", "Unsupported coordinate system type");
			return Result;
	}

	return Result;
}


void Daylon::FLevellerDocument::ComputeSpan()
{
	check(DataLength != 0 && Width != 0 && Breadth != 0);

	float* HfPixelsPtr = (float*)(Contents.GetData() + DataOffset);

	SpanLow = SpanHi = HfPixelsPtr[0];

	for(int32 I = 0; I < Width * Breadth; I++)
	{
		SpanLow = FMath::Min(SpanLow, HfPixelsPtr[I]);
		SpanHi  = FMath::Max(SpanHi,  HfPixelsPtr[I]);
	}
}


bool Daylon::FLevellerDocument::Read(uint64 Length, void* Buffer)
{
	if(Buffer == nullptr)
	{
		return false;
	}

	if(Length > MAX_uint32)
	{
		return false;
	}

	if(Mark + Length >= (uint64)Contents.Num())
	{
		return false;
	}

	FMemory::Memcpy(Buffer, Contents.GetData() + Mark, (SIZE_T)Length);

	Mark += Length;

	return true;
}

		
bool Daylon::FLevellerDocument::LocateData(const char* Tag, uint64& Offset, uint64& Length)
{
	// Locate the file offset of the desired tag's data.
	// If it is not available, return false.
	// If the tag is found, leave the filemark at the start of its data.

	Mark = 5;

	const int kMaxDescLen = 64;

	for (;;)
	{
				
		uint8 DescriptorLen;
		if(!Read(1, &DescriptorLen)) return false;

		if (DescriptorLen == 0 || DescriptorLen > (size_t)kMaxDescLen)
			return false;

		char Descriptor[kMaxDescLen + 1];
		if(!Read(DescriptorLen, &Descriptor))
			return false;

		Descriptor[DescriptorLen] = 0;

		uint32 BlockLength;
		if(!Read(4, &BlockLength)) return false;

		if(0 == strcmp(Descriptor, Tag))
		{
			Length = BlockLength;
			Offset = Mark;
			return true;
		}
		else
		{
			// Seek to next tag.
			Mark += BlockLength;
		}
	}
}


bool Daylon::FLevellerDocument::Get(const char* Tag, int32& N, FLandscapeFileInfo& Result)
{
	uint64 Offset;
	uint64 Length;

	if(!LocateData(Tag, Offset, Length) || Length != sizeof(int32))
	{
		Result.ResultCode = ELandscapeImportResult::Error;
		Result.ErrorMessage = LOCTEXT("DaylonLeveller_TagNotFoundError", "Leveller document missing necessary data");
		return false;
	}
			
	return Read(Length, &N);
}


bool Daylon::FLevellerDocument::Get(const char* Tag, double& N, FLandscapeFileInfo& Result)
{
	uint64 Offset;
	uint64 Length;

	if(!LocateData(Tag, Offset, Length) || Length != sizeof(double))
	{
		Result.ResultCode = ELandscapeImportResult::Error;
		Result.ErrorMessage = LOCTEXT("DaylonLeveller_TagNotFoundError", "Leveller document missing necessary data");
		return false;
	}
			
	return Read(Length, &N);
}



class FDaylonLevellerHeightmapFileFormat : public ILandscapeHeightmapFileFormat 
{
	private:
		FLandscapeFileTypeInfo FileTypeInfo;

	public:
		FDaylonLevellerHeightmapFileFormat()
		{
			FileTypeInfo.Description = LOCTEXT("FileFormatDaylonLeveller_HeightmapDesc", "Daylon Leveller documents");
			FileTypeInfo.Extensions.Add(".ter");
			FileTypeInfo.bSupportsExport = false;
		}

		virtual const FLandscapeFileTypeInfo& GetInfo() const override
		{
			return FileTypeInfo;
		}

		virtual FLandscapeFileInfo Validate(const TCHAR* HeightmapFilename, FName LayerName) const override
		{
			Daylon::FLevellerDocument Document;
			return Document.Init(HeightmapFilename);
		}

		virtual FLandscapeImportData<uint16> Import(const TCHAR* HeightmapFilename, FName LayerName, FLandscapeFileResolution ExpectedResolution) const override
		{
			Daylon::FLevellerDocument Document;
			auto InitResult = Document.Init(HeightmapFilename);

			FLandscapeImportData<uint16> Result;

			Result.ResultCode = ELandscapeImportResult::Success;

			if(InitResult.ResultCode != ELandscapeImportResult::Success)
			{
				Result.ResultCode   = InitResult.ResultCode;
				Result.ErrorMessage = InitResult.ErrorMessage;
				return Result;
			}

			if(Document.Width != ExpectedResolution.Width || Document.Breadth != ExpectedResolution.Height)
			{
				Result.ResultCode = ELandscapeImportResult::Error;
				Result.ErrorMessage = LOCTEXT("DaylonLeveller_ResolutionMismatch", "The Leveller document's resolution does not match the requested resolution");
				return Result;
			}

			const float Height = Document.SpanHi - Document.SpanLow;

			if(Height == 0.0f)
			{
				// Document is flat, so just set all zeros into the result.
				Result.Data.SetNum(0);
				Result.Data.AddZeroed(Document.Width * Document.Breadth);
				return Result;
			}

			// Document is not flat; map heights to full uint16 range. todo: preserve slope and use Z scale.

			float* HfPixels = (float*)(Document.Contents.GetData() + Document.DataOffset);

			Result.Data.SetNum(Document.Width * Document.Breadth);

			for(int32 I = 0; I < Document.Width * Document.Breadth; I++)
			{
				Result.Data[I] = FMath::RoundToInt(((HfPixels[I] - Document.SpanLow) / Height) * 0xFFFF);
			}

			return Result;
		}

		virtual void Export(const TCHAR* HeightmapFilename, FName LayerName, TArrayView<const uint16> Data, FLandscapeFileResolution DataResolution, FVector Scale) const override
		{
			// todo
		}
};

// todo: weightmap format?


void FDaylonLevellerLandscapeModule::StartupModule()
{
	auto& ModuleManager = FModuleManager::Get();

	ModuleManager.LoadModuleChecked(TEXT("LandscapeEditor"));

	auto& Module = ModuleManager.GetModuleChecked<ILandscapeEditorModule>(FName("LandscapeEditor"));

	Module.RegisterHeightmapFileFormat(MakeShareable(new FDaylonLevellerHeightmapFileFormat()));
}


void FDaylonLevellerLandscapeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FDaylonLevellerLandscapeModule, DaylonLevellerLandscape)