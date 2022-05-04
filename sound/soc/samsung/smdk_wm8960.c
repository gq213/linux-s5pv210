// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <linux/clk.h>

#include "i2s.h"

struct smdk_priv {
	struct clk *clk_fout_epll;
};

static int smdk_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct snd_soc_card *card = rtd->card;
	struct smdk_priv *priv = snd_soc_card_get_drvdata(card);
	unsigned int width, frmclk;
	unsigned int bfs, rfs, rclk, psr;
	unsigned long clk_rate, old_clk_rate;
	int ret;

	width = params_width(params);
	frmclk = params_rate(params);
	dev_info(card->dev, "%s: width=%u, frmclk=%u\n", __func__, width, frmclk);
	
	if (width == 16) {
		bfs = 32;
		rfs = 256;
	} else if (width == 24) {
		bfs = 48;
		rfs = 384;
	} else {
		dev_err(card->dev, "%s: width not supported\n", __func__);
		return -EINVAL;
	}
	dev_info(card->dev, "%s: bfs=%u, rfs=%u\n", __func__, bfs, rfs);
	
	rclk = frmclk * rfs;
	dev_info(card->dev, "%s: rclk=%u\n", __func__, rclk);

	switch (rclk) {
	case 2048000:
	case 2822400:
	case 3072000:
	case 4233600:
	case 4608000:
		psr = 16;
		break;

	case 4096000:
	case 5644800:
	case 6144000:
	case 8467200:
	case 9216000:
		psr = 8;
		break;

	case 8192000:
	case 11289600:
	case 12288000:
	case 16934400:
	case 18432000:
		psr = 4;
		break;

	case 16384000:
	case 22579200:
	case 24576000:
	case 33868800:
	case 36864000:
		psr = 2;
		break;

	case 32768000:
	case 45158400:
	case 49152000:
	case 67737600:
	case 73728000:
		psr = 1;
		break;

	default:
		dev_err(card->dev, "%s: frmclk not supported\n", __func__);
		return -EINVAL;
	}
	dev_info(card->dev, "%s: psr=%u\n", __func__, psr);
	
	clk_rate = rclk * psr;
	old_clk_rate = clk_get_rate(priv->clk_fout_epll);
	
	if (old_clk_rate != clk_rate) {
		dev_info(card->dev, "%s: fout_epll is %lu, wanted %lu\n", __func__, old_clk_rate, clk_rate);
		clk_set_rate(priv->clk_fout_epll, clk_rate);
		old_clk_rate = clk_get_rate(priv->clk_fout_epll);
		dev_info(card->dev, "%s: fout_epll want %lu, got %lu\n", __func__, clk_rate, old_clk_rate);
	} else {
		dev_info(card->dev, "%s: fout_epll = %lu\n", __func__, old_clk_rate);
	}
	
	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_OPCLK, 0, SAMSUNG_I2S_OPCLK_PCLK);
	if (ret < 0) {
		dev_err(card->dev, "%s: AP OPCLK setting error, %d\n", __func__, ret);
		return ret;
	}
	
	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_RCLKSRC_1, 0, 0);
	if (ret < 0) {
		dev_err(card->dev, "%s: AP RCLKSRC_1 setting error, %d\n", __func__, ret);
		return ret;
	}
	
	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_CDCLK, rfs, SND_SOC_CLOCK_OUT);
	if (ret < 0) {
		dev_err(card->dev, "%s: AP CDCLK setting error, %d\n", __func__, ret);
		return ret;
	}

	ret = snd_soc_dai_set_clkdiv(cpu_dai, SAMSUNG_I2S_DIV_BCLK, bfs);
	if (ret < 0) {
		dev_err(card->dev, "%s: AP BCLK setting error, %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

static struct snd_soc_ops smdk_ops = {
	.hw_params = smdk_hw_params,
};

static int smdk_init(struct snd_soc_pcm_runtime *runtime)
{
	struct snd_soc_card *card = runtime->card;
	
	dev_info(card->dev, "%s\n", __func__);
	
	return 0;
}

SND_SOC_DAILINK_DEFS(pcm,
	DAILINK_COMP_ARRAY(COMP_EMPTY()),
	DAILINK_COMP_ARRAY(COMP_CODEC(NULL, "wm8960-hifi"), COMP_CODEC(NULL, "i2s-hifi")),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));

static struct snd_soc_dai_link smdk_dailink = {
	.name 		 = "wm8960",
	.stream_name = "wm8960 PCM",
	.init 		 = smdk_init,
	.ops 		 = &smdk_ops,
	.dai_fmt     = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
				   SND_SOC_DAIFMT_CBS_CFS,
	SND_SOC_DAILINK_REG(pcm),
};

static struct snd_soc_card smdk = {
	.name = "SMDK-I2S",
	.owner = THIS_MODULE,
	.dai_link = &smdk_dailink,
	.num_links = 1,
};

static int smdk_audio_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct snd_soc_card *card = &smdk;
	struct device_node *np = pdev->dev.of_node;
	struct smdk_priv *priv;
	const char *clk_name;

	if (!np) {
		dev_err(&pdev->dev, "np invalid\n");
		return -EINVAL;
	}

	card->dev = &pdev->dev;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&pdev->dev, "devm_kzalloc priv failed\n");
		return -ENOMEM;
	}

	snd_soc_card_set_drvdata(card, priv);
	
	clk_name = "fout_epll";
	priv->clk_fout_epll = devm_clk_get(&pdev->dev, clk_name);
	if (IS_ERR(priv->clk_fout_epll)) {
		dev_err(&pdev->dev, "Failed to get %s\n", clk_name);
		return PTR_ERR(priv->clk_fout_epll);
	}

	smdk_dailink.codecs[0].of_node = of_parse_phandle(np,
			"samsung,audio-codec", 0);
	if (!smdk_dailink.codecs[0].of_node) {
		dev_err(&pdev->dev,
			"Property 'samsung,audio-codec[0]' missing or invalid\n");
		return -EINVAL;
	}

	smdk_dailink.codecs[1].of_node = of_parse_phandle(np,
			"samsung,audio-codec", 1);
	if (!smdk_dailink.codecs[1].of_node) {
		dev_err(&pdev->dev,
			"Property 'samsung,audio-codec[1]' missing or invalid\n");
		smdk_dailink.num_codecs -= 1;
	} else if (!of_device_is_available(smdk_dailink.codecs[1].of_node)) {
		dev_err(&pdev->dev,
			"Property 'samsung,audio-codec[1]' disabled\n");
		smdk_dailink.num_codecs -= 1;
	}

	smdk_dailink.cpus->of_node = of_parse_phandle(np,
			"samsung,i2s-controller", 0);
	if (!smdk_dailink.cpus->of_node) {
		dev_err(&pdev->dev,
			"Property 'samsung,i2s-controller' missing or invalid\n");
		ret = -EINVAL;
		goto put_codec_of_node;
	}

	smdk_dailink.platforms->of_node = smdk_dailink.cpus->of_node;

	/* Update card-name if provided through DT, else use default name */
	ret = snd_soc_of_parse_card_name(card, "samsung,model");
	if (ret) {
		dev_warn(&pdev->dev,
			"Soc parse card name failed %d\n", ret);
	}

	/* register the soc card */
	ret = devm_snd_soc_register_card(&pdev->dev, card);
	if (ret) {
		dev_err(&pdev->dev,
			"Soc register card failed %d\n", ret);
		goto put_cpu_of_node;
	}
	
	dev_info(&pdev->dev, "%s\n", __func__);

	return 0;

put_cpu_of_node:
	of_node_put(smdk_dailink.cpus->of_node);
	smdk_dailink.cpus->of_node = NULL;
put_codec_of_node:
	of_node_put(smdk_dailink.codecs->of_node);
	smdk_dailink.codecs->of_node = NULL;

	return ret;
}

static int smdk_audio_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "%s\n", __func__);

	of_node_put(smdk_dailink.cpus->of_node);
	smdk_dailink.cpus->of_node = NULL;
	of_node_put(smdk_dailink.codecs->of_node);
	smdk_dailink.codecs->of_node = NULL;

	return 0;
}

static const struct of_device_id samsung_wm8960_of_match[] = {
	{ .compatible = "samsung,smdk-wm8960", },
	{},
};
MODULE_DEVICE_TABLE(of, samsung_wm8960_of_match);

static struct platform_driver smdk_audio_driver = {
	.probe = smdk_audio_probe,
	.remove = smdk_audio_remove,
	.driver = {
		.name = "smdk-audio-wm8960",
		.pm	= &snd_soc_pm_ops,
		.of_match_table = samsung_wm8960_of_match,
	},
};

module_platform_driver(smdk_audio_driver);

MODULE_AUTHOR("gaoqiang1211@gmail.com");
MODULE_DESCRIPTION("ALSA SoC SMDK WM8960");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" "smdk-audio-wm8960");
